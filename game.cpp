/*
CODE BY CHRISTIAN LO, 1498360 AND KELLY LUC, 1498694
Basis of code is based off of previous lectures.

This code displays a map and find restaurant closest to the cursor
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "lcd_image.h"

// TFT display and SD card will share the hardware SPI interface.
// For the Adafruit shield, these are the default.
// The display uses hardware SPI, plus #9 & #10
// Mega2560 Wiring: connect TFT_CLK to 52, TFT_MISO to 50, and TFT_MOSI to 51
#define TFT_DC  9
#define TFT_CS 10
#define SD_CS   6

//BUZZER pin
#define BUZZ_PIN 12

// calibration data for the touch screen, obtained from documentation
// the minimum/maximum possible readings from the touch point
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

// thresholds to determine if there was a touch
#define MINPRESSURE   10
#define MAXPRESSURE 1000

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

//display dimensions
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240

//joystick control
#define JOY_VERT  A1 // should connect A1 to pin VRx
#define JOY_HORIZ A0 // should connect A0 to pin VRy
#define JOY_SEL   2

#define JOY_CENTER   512
#define JOY_DEADZONE 64
#define TUNING       200

//paddle dimensions
#define PADDLE_WIDTH 56
#define PADDLE_HEIGHT 8
#define PADDLE_Y_DISPLACEMENT 15
#define BALL_SIZE 5

#define DELAY 10
//SD card
Sd2Card card;

//initializing the game mode to start the game
int mode = 0;

// the coordinate of the paddle
int cursorX;
int cursorY;
#define CURSOR_SIZE 9

File file;

//initializing setup of the arduino
void setup() {
  init();

  Serial.begin(9600);
  //setup joystick
  pinMode(JOY_SEL, INPUT_PULLUP);
  //setup buzzer
  pinMode(BUZZ_PIN, OUTPUT);
  // Checking if there is a SD card
  tft.begin();
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed! Is it inserted properly?");
    while (true) {}
  }
  Serial.println("OK!");

  // checking is the card has the the required data
  Serial.print("Initializing SPI communication for raw reads...");
  if (!card.init(SPI_HALF_SPEED, SD_CS)) {
    Serial.println("failed! Is the card inserted properly?");
    while (true) {}
  }

  else {
    Serial.println("OK!");
  }

  // setting the orientation of the LCD screen
  tft.setRotation(3);

  // clears the screen by filling it black
  tft.fillScreen(ILI9341_BLACK);
}
//drawBricks
#define BRICK_WIDTH 38
#define BRICK_HEIGHT 12
#define BRICK_COUNT_ROWS 6
#define BRICK_COUNT_COLS 8


//length of a 'beep' when buzzer plays
#define BEEP 25


//Array of colours for bricks each row representing a colur
uint32_t brickColour[6] = {ILI9341_MAGENTA,ILI9341_RED,ILI9341_YELLOW,ILI9341_GREEN,ILI9341_CYAN,ILI9341_BLUE};

//Keeps count of player score
int score = 0;

//Checks if brick is destroyed, if it true remove brick and no collision occur
// afterwards
bool isBrickDestroyed [BRICK_COUNT_ROWS] [BRICK_COUNT_COLS];

//Brick destruction relies on
void resetHitMatrix(){
	for(int i=0 ; i<BRICK_COUNT_ROWS; i++){
		for (int j=0;j<BRICK_COUNT_COLS;j++){
			isBrickDestroyed[i][j] = false;
		}
	}
}

//draws the bricks
void drawBricks(){
	int yBrickPos = 35;
	//i is rows and j is columns of the bricks each being printed
	for(int i=0 ; i<BRICK_COUNT_ROWS; i++){
		int xBrickPos = 1;
		for (int j=0;j<BRICK_COUNT_COLS;j++){

      //checks if brick is destroyed,if not print the brick
      if (isBrickDestroyed[i][j]==false){
        tft.fillRect(xBrickPos,yBrickPos,BRICK_WIDTH,BRICK_HEIGHT,brickColour[i]);
      }

			xBrickPos+=BRICK_WIDTH+2;
		}
		yBrickPos += BRICK_HEIGHT+2;
	}
}

#define SCORE_SIZE 2

//Spacing of the score
int scoreSpacing = SCORE_SIZE * 5;


//Movement of the ball
int ballX = DISPLAY_WIDTH/2 +70;
int ballY = DISPLAY_HEIGHT/2 +20;
int ballDx = -2;
int ballDy = 2;
int startPause = 1;

//set up mode one
void mode0Setup(){

  //Pause the screen so player can get ready to play
  startPause = 1;

  //Placement of the ball
  ballX = DISPLAY_WIDTH/2 +70;
  ballY = DISPLAY_HEIGHT/2 +20;

  //Ball's direction
  ballDx = -2;
  ballDy = 2;

  //Printing the text score
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0,0);
  tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
  tft.setTextSize(SCORE_SIZE);
  tft.setTextWrap(false);
  tft.print("Score:");
	cursorX = DISPLAY_WIDTH/2;
	resetHitMatrix();
	drawBricks();
}
//--------------------MODE1-----------------------------

//Setting up mode 1 "Game over" allowing player to input their name
// with their score
int textColour = 0;
int textPos = DISPLAY_WIDTH/6;
int letterSel = 65;
int playerChar = 0;

//"Game over" screen it prints insults to the user
char insults[5][45]= {"Ha, loser.","It's time to stop.",
              "Try harder.","WINNERS DON'T DO DRUGS","int IQ = yourScore;"};

//Setup of mode 1
void mode1Setup(){

  // Ascii value of the capital letter A
  letterSel = 65;

  //Counter to keep track player name input of 3
  playerChar = 0;
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0,0);
  tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
  tft.setTextWrap(true);
  tft.setTextSize(2);

  //Printing score and name
  tft.println(insults[random(0,5)]);
  tft.setTextSize(2);
  tft.print("Your score: ");tft.print(score);
  tft.setTextSize(7);

  textPos = DISPLAY_WIDTH/6;
  tft.setCursor(textPos,DISPLAY_HEIGHT/2);
  for (int i=0 ; i<3; i++){
    tft.print('A');
    tft.print(' ');
  }
  tft.setCursor(textPos,DISPLAY_HEIGHT/2);
}

//Creates an array to store name
char playerName[4];

//Jyystick controls in the "Game over" screen
// scrolling through the alphabet and comfirming their name
char processJoystickUI(){
  // movement of the joystickproc
  int yVal = analogRead(JOY_VERT);
  int button = digitalRead(JOY_SEL);
  //defined new vairables to prevent flickering. Current inplementation looks if
  //cursor is moving, and if so, prints the map over the part the cursor WAS on.
  //int cursorXOld = cursorX;
  int cursorYOld = cursorY;

  char control;
  //Button pulled up so pressed = 0
  //s is select

  //Comfirmation
  if (button == 0){
    control = 's';
  }

  //Down
  else if (yVal < JOY_CENTER - JOY_DEADZONE) {
    //cursorY -= 1 + abs(yVal-JOY_CENTER)/TUNING; // decrease the y coordinate of the cursor
    control= 'd';
  }

  //Up
  else if (yVal > JOY_CENTER + JOY_DEADZONE) {
    // += 1 + abs(yVal-JOY_CENTER)/TUNING;
    control = 'u';
  }
  //Nothing. x is "null".
  else{
    control = 'x';
  }
  return control;

}

// create a structure to use when stroing values in the SD card
struct nameScore{
  char name[4];
  int score;
};

#define SCOREBOARD_SIZE 15

nameScore scoreBoard[SCOREBOARD_SIZE];

//Reads the SD card with the list of Hi-scores
void readFromSD(){
  //Read data from file
  file = SD.open("SCORE.txt",FILE_READ);
  if(file){
    int scoreBoardCounter = 0;

    while(file.available() && scoreBoardCounter!=15){

      char byteRead = (char) file.read();
      //We know names are three letters long, so reading it will be "hardcoded".
      int nameCount = 0;
      while((byteRead != '\r')&&(file.available())){
        scoreBoard[scoreBoardCounter].name[nameCount] = byteRead;
        byteRead = (char) file.read();
        nameCount++;
      }
      //null terminate
      scoreBoard[scoreBoardCounter].name[4] = '\0';

      //Skip the new line characters
      byteRead = (char) file.read();
      byteRead = (char) file.read();

      //Now read characters until a "\r" is hit.
      int scoreCharIndex = 0;

      //up to 6 digits for a score.Last digit is for null-terminator.
      char scoreChars[7] = "";
      while ((byteRead != '\r')&&(file.available())){
        Serial.print(byteRead);
        scoreChars[scoreCharIndex] = byteRead;
        scoreCharIndex++;
        byteRead = (char) file.read();
      }

      byteRead = (char) file.read();

      //now convert obtained score string into an int, and store in struct
      scoreBoard[scoreBoardCounter].score = atoi(scoreChars);
      scoreBoardCounter++;
    }
    Serial.println("Done Reading...");
    file.close();
  }
  else{
    Serial.println("Error! Did not read file");
  }
}

//Test cases to check if SD reads and writes
void SDCardTest(){
  for (int i = 0; i<90;i++){
    Serial.print(i);Serial.print(": NAME: ");Serial.println(scoreBoard[i].name);
    Serial.print("   SCORE: ");Serial.println(scoreBoard[i].score);
  }

}


// Swap two of nameScore struct
// Swap rest used from the 2nd assignment
void swap_rest(nameScore *ptr_rest1, nameScore *ptr_rest2) {
  nameScore tmp = *ptr_rest1;
  *ptr_rest1 = *ptr_rest2;
  *ptr_rest2 = tmp;
}

// Pivot used to sort the score
int pivot( nameScore *a, int n, int pi) {
	int lo = pi;
	int hi= pi+n-2;
  int index = pi+n;

	swap_rest(&a[index-1],&a[pi+(n/2)]);
	int counter = 0;

  // organizing the score from highest to lowest
	while(lo <= hi){

		if(a[lo].score >= a[index-1].score){
			lo++;
		}
		else if(a[hi].score < a[index-1].score){
			hi--;
		}
		else{
			swap_rest(&a[lo],&a[hi]);
		}
		counter++;
	}
	swap_rest(&a[index-1],&a[lo]);
	return lo;
}

// Sort an array with n elements using Quick Sort
void qsort(nameScore *scoreBoard, int n, int piv) {
	// if n <= 1 do nothing (just return)
	if(n<= 1){
		return;
	}

	// pick a pivot index pi
	int npiv = pivot(scoreBoard,n,piv);

	// call pivot with this pivot index, store the returned
	// location of the pivot in a new variable, say new_pi

	// recursively call qsort twice:
	// - once with the part before index new_pi
		qsort(scoreBoard,npiv,0);
	// - once with the part after index new_pi
		qsort(scoreBoard,n-npiv,npiv);
	// and that’s it!
}

// writes to SD card and store in the H-score
void writeToSD(){
  //Read data from file
  file = SD.open("SCORE.txt",FILE_WRITE);
  if(file){
    //set cursor to the very start of file; start overriding
    file.seek(0);

  for (int j = 0; j < 15; j++){
    for(int i = 0; i < 3; i++){
      file.print(scoreBoard[j].name[i]);
    }
    file.println("");
    file.println(scoreBoard[j].score);
  }

    Serial.println("Done Writing...");
    file.close();
  }
  else{
    Serial.println("Error! Did not read file");
  }
}

//Setup of mode 2, displaying hi-score
void mode2Setup(){

  //fill scoreBoard
  for(int i =0; i<SCOREBOARD_SIZE; i++){
    scoreBoard[i].score = -1;
  }

  //Read From SD
  readFromSD();

  //Replace worst score with players score, as worst score is never displayed.
  scoreBoard[SCOREBOARD_SIZE-1].score = score;
  for (int i=0; i<4;i++){
    scoreBoard[SCOREBOARD_SIZE-1].name[i] = playerName[i];
  }

  //Sort the scoreBoard using quick sort
  qsort(scoreBoard,SCOREBOARD_SIZE,0);

  tft.fillScreen(0);
  tft.setTextSize(2);
  tft.setCursor(0,0);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextWrap(false);

  Serial.println(scoreBoard[5].name);
  Serial.println(playerName);

  //boolean that checks if user highlighting has been applied yet,if so, prevents
  //highlighting of any duplicate nameScores.
  bool alreadyShown = false;

  //Printing out the score
  for (int i =0; i<15;i++){
      //Player will be highlighted in red
      if((scoreBoard[i].name[0] == playerName[0]) && (scoreBoard[i].name[1] == playerName[1]) &&
      (scoreBoard[i].name[2] == playerName[2]) && (scoreBoard[i].score == score) && !alreadyShown){
        alreadyShown = true;
        tft.setTextColor(ILI9341_RED);
      }
      tft.print(scoreBoard[i].name);tft.print(":                  ");tft.print(scoreBoard[i].score);tft.print("\n");
      tft.setTextColor(ILI9341_GREEN);
  }
}

//mode 2: hi-score screen
void mode2(){


  char input = processJoystickUI();

  //Click to replay
  if (input == 's'){
    //store the scores away
    writeToSD();
    score = 0;
    mode = 0;
    tone(BUZZ_PIN,800,BEEP);
    mode0Setup();
  }
}

//mode 1: inputting name
void mode1(){

  //Frequently changes colour of the text
  if (textColour < 5){
    textColour++;
  }
  else if (textColour == 5){
    textColour = 0;
  }
  tft.setCursor(textPos,DISPLAY_HEIGHT/2);
  tft.setTextColor(brickColour[textColour],ILI9341_BLACK);

  //control statements
  char input = processJoystickUI();

  //Have a reel of Chars of capital letters. Use joystick to cycle through.
  if (input == 'u'){
    if (letterSel == 90){
      letterSel = 65;
    }
    else{
      letterSel++;
    }
    tone(BUZZ_PIN,700,BEEP);
  }

  if (input == 'd'){
    if (letterSel == 65){
      letterSel = 90;
    }
    else{
      letterSel--;
    }
    tone(BUZZ_PIN,700,BEEP);
  }

  //if button is pushed, store chars inputted by player. Does this three times
  if (input == 's'){
    if (playerChar < 3){
      playerName[playerChar] = (char) letterSel;
      playerChar++;
      textPos += 84;
      tft.setCursor(textPos,DISPLAY_HEIGHT/2);
      letterSel = 65;

      tone(BUZZ_PIN,800,BEEP);

      //pseudo-debouncing
      delay(85);
    }
  }

  //Return when three characters are selected.
  if(playerChar == 3){
    //Null terminate array
    playerName[3]= '\0';
    Serial.println(playerName);
    mode = 2;
    mode2Setup();
    return;
  }

  //print
  tft.print((char)letterSel);
  delay(160);
}

bool gameCleared = false;

//the game itself where the ball move and hits the bricks,
//the player cant let the ball hit the bottom of the screen
void gameLoop(){
  tft.setCursor(scoreSpacing*7,0);
  tft.print(score);
	int ballXOld = ballX;
	int ballYOld = ballY;

	tft.fillRect(ballXOld - BALL_SIZE/2, ballYOld - BALL_SIZE/2, BALL_SIZE, BALL_SIZE, ILI9341_BLACK);

	//Basic Ball Deflection Conditions on the boudary of the screen
	if ((ballX<0+BALL_SIZE/2)||(ballX > DISPLAY_WIDTH-BALL_SIZE/2)){
		ballDx = ballDx * -1;
		ballX = constrain(ballX,0+BALL_SIZE/2,DISPLAY_WIDTH-BALL_SIZE/2);
	}
	if (ballY<0+BALL_SIZE/2){
		ballDy = ballDy * -1;
		ballY = constrain(ballY,0+BALL_SIZE/2,DISPLAY_HEIGHT-BALL_SIZE/2);
	}

  //Loss condition, changes mode if ball hits the bottom of the screen
  if (ballY > DISPLAY_HEIGHT-BALL_SIZE/2){
    tone(BUZZ_PIN,800,BEEP);
    mode = 1;
    mode1Setup();
  }

	//Collision checking with paddle
	if (ballY+BALL_SIZE == DISPLAY_HEIGHT-PADDLE_Y_DISPLACEMENT){
		if((ballX>cursorX - PADDLE_WIDTH/2)&&(ballX<cursorX+PADDLE_WIDTH/2)){
			ballDy = ballDy *-1;
      tone(BUZZ_PIN,1000,BEEP);
		}

	}

/*
  if(((ballY+BALL_SIZE/2>cursorY - PADDLE_HEIGHT/2)&&(ballY+BALL_SIZE/2<cursorY+PADDLE_HEIGHT/2))
  ||((ballY-BALL_SIZE/2>cursorY - PADDLE_HEIGHT/2)&&(ballY-BALL_SIZE/2<cursorY+PADDLE_HEIGHT/2))){
    ballDx = ballDx *-1;
  }
*/



  //balls movement
	ballX += ballDx;
	ballY += ballDy;
	tft.fillRect(ballX - BALL_SIZE/2, ballY - BALL_SIZE/2, BALL_SIZE, BALL_SIZE, ILI9341_WHITE);

  //pauses the game initially so that the player can start whenever they
  // are ready
  while(startPause == 1){
    //Don't run game until joystick button is pushed.
    tft.setCursor(DISPLAY_WIDTH/2-80,DISPLAY_HEIGHT/2 +60);
    tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
    tft.setTextSize(2);

    // prompt the player to start
    tft.print("Push to start");
    startPause = digitalRead(JOY_SEL);
    if(startPause == 0){
      tft.fillRect(0,DISPLAY_HEIGHT/2+60,DISPLAY_WIDTH,16,ILI9341_BLACK);
    }
  }

  //Brick collision___________________

  //if all bricks a destroyed game replay so player
  // proceed and get a hi-score
  gameCleared = true;

  int yBrickPos = 35;
	//i is rows and j is columns
	for(int i=0 ; i<BRICK_COUNT_ROWS; i++){
		int xBrickPos = 1;
		for (int j=0;j<BRICK_COUNT_COLS;j++){
      bool isBrickHit = false;
      //loop to see if ball is clipped into the cube
      if (isBrickDestroyed[i][j]== true){
        xBrickPos+=BRICK_WIDTH+2;
        continue;
      }
      //Checks if ball is within brick dimensions
      if ((ballX-BALL_SIZE/2<=xBrickPos+BRICK_WIDTH)&&(ballX+BALL_SIZE/2>=xBrickPos)){
        if ((ballY-BALL_SIZE/2<=yBrickPos+BRICK_HEIGHT)&&(ballY+BALL_SIZE/2>=yBrickPos)){

          if ((ballX>=xBrickPos+BRICK_WIDTH)||(ballX<=xBrickPos)){
            //changes direction if the ball hits it horizontally
            ballDx = ballDx*-1;
          }
          else if ((ballY>=yBrickPos+BRICK_HEIGHT)||(ballY<=yBrickPos)){
            //changes direction if the ball hits it vertically
            ballDy = ballDy*-1;
          }
          isBrickHit = true;
        }
      }

      //brick get destroyed when ball collides with it
      if((isBrickDestroyed[i][j]==false)&&(isBrickHit==true)){
        isBrickDestroyed[i][j] = true;
        tone(BUZZ_PIN,1500,BEEP);
        score++;

        tft.fillRect(xBrickPos,yBrickPos,BRICK_WIDTH,BRICK_HEIGHT,ILI9341_BLACK);
      }

      //checks if bricks are all destroyed. If true,reset the board.
      if(isBrickDestroyed[i][j]==false){
        gameCleared = false;
      }

      //increment bricks postion
			xBrickPos+=BRICK_WIDTH+2;
		}
		yBrickPos += BRICK_HEIGHT+2;
	}

  if (gameCleared == true){
    //reset the game without clearing the score
    resetHitMatrix();
    mode0Setup();
  }
  delay(DELAY);

}

// make paddle cycle through different colours
int paddleColour = 0;
void redrawCursor(uint16_t colour) {
  if (paddleColour < 6){
    paddleColour++;
  }
  else if (paddleColour == 6){
  paddleColour = 0;
  }
  // creating the cursor of the screen
  tft.fillRect(cursorX - PADDLE_WIDTH/2, DISPLAY_HEIGHT - PADDLE_Y_DISPLACEMENT - PADDLE_HEIGHT/2,
    PADDLE_WIDTH,PADDLE_HEIGHT, brickColour[paddleColour]);

}

//captures the movement of the joystick, and moves the cursor (or the screen position)
//accordingly
void processJoystickGame() {

  // movement of the joystick
  int xVal = analogRead(JOY_HORIZ);

  //defined new vairables to prevent flickering. Current inplementation looks if
  //cursor is moving, and if so, prints the map over the part the cursor WAS on.
  int cursorXOld = cursorX;
  //int cursorYOld = cursorY;

  // checks if cursor moved
  bool isCursorMoving = false;

  // now move the cursor
  //NOTE: The cursor now varies depending on how far the joystick goes out, by taking the
  //distance it is from the centre and dividing it by a "tuning" constant that can be adjusted
  //if so needed.

  if (xVal > JOY_CENTER + JOY_DEADZONE) {
    cursorX -= 1 + abs(xVal-JOY_CENTER)/TUNING;
    isCursorMoving = true;
  }

  else if (xVal < JOY_CENTER - JOY_DEADZONE) {
    cursorX += 1 + abs(xVal-JOY_CENTER)/TUNING;
    isCursorMoving = true;
  }

  //If cursor is moving, draw where the cursor was, else don't redraw map on old position
  //Makes cursor "float" and also prevents idle flickering
  if (isCursorMoving == true){
		tft.fillRect(cursorXOld - PADDLE_WIDTH/2, DISPLAY_HEIGHT - PADDLE_Y_DISPLACEMENT - PADDLE_HEIGHT/2,
	    PADDLE_WIDTH,PADDLE_HEIGHT, ILI9341_BLACK);
  }

	//constrain the position of the cursor between the confines of the screen.
	cursorX = constrain(cursorX, 0 + (PADDLE_WIDTH/2),DISPLAY_WIDTH-(PADDLE_WIDTH/2));
	redrawCursor(ILI9341_WHITE);

}


int main(){
  //initializing setup for the game
	setup();
	mode0Setup();

  //states of the game
  //mode 0: is the game itself where player has to destroy all bricks
  // and get a hi-score
  //mode 1: once player loses the get to write a 3 letter name
  // with their hi-score
  //mode 2: displaying current top hi-score
  while(true){

  	while (mode == 0){
  	  processJoystickGame();
  	  gameLoop();
  	}
    while (mode == 1){
      mode1();
    }
    while (mode == 2){
      mode2();
    }
  }
	Serial.end();

  //closes the file after it updates the new hi-score listing
  file.close();
	return 0;
}
