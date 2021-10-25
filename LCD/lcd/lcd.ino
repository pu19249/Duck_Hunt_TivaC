/*
   DUCK HUNT IN TIVA C TM4C123
   Marco Duarte - Jonathan Pu
   With the advice of Jose Morales and Diego Morales
   Hardware: Tiva-c with Ili9341 to display the game
   using a MicroSD to store backgrounds
   Another Tiva c to use it as a controler using any
   communication protocol and an Arduino Leonardo for the same
   Software:
   1. Main screen with intro melody
   2. Menu screen to select player 1 and player 2
   3. Screen in which P1 can shot 3 times
   4. Then P2 can shot 3 times
*/

//********************************************************
//                      LIBRARIES
//********************************************************
#include <stdint.h>
#include <stdbool.h>
#include <TM4C123GH6PM.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "font.h"
#include "lcd_registers.h"
#include <SPI.h>
#include <SD.h>
#include "pitches.h"
//*****************************************************************
//              MELODIES DEFINITIONS
//*****************************************************************
void tone_function(int mel[], int duration[], int quant);
int input = 0;
//Intro music
int melody_intro[] = { NOTE_GS3, NOTE_A3, 0, NOTE_G3, 0, 0, NOTE_FS3, NOTE_F3, NOTE_E3, 0, 0, NOTE_A3, NOTE_DS3,
                       0, NOTE_C4, NOTE_A3, NOTE_DS3, NOTE_C3, NOTE_B3, NOTE_GS3, NOTE_A3, NOTE_G3, 0, 0, NOTE_A3, NOTE_A3
                     }; //26

//Main menu melody
int melody_menu[] = {NOTE_C4, NOTE_F4, NOTE_A4, NOTE_F4, NOTE_F4,
                     NOTE_C4, NOTE_F4, NOTE_A4, NOTE_G4,
                     NOTE_A4, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_F4
                    }; //16

//Failed shot
int melody_failed[] = {NOTE_G4, NOTE_C4}; //LOSE SOUND 2


//Winner
int melody_winner[] = {NOTE_B5, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_E5}; //8

//Game over music
int melody_over[] = {NOTE_DS5, NOTE_D5, NOTE_CS5}; //GAME OVER 3


//No more bullets
int melody_bull[] = {NOTE_D3}; //1


// note durations: 4 = quarter note, 8 = eighth note, etc.:
//Intro music
int noteDurations_intro[] = {12, 12, 12, 12, 2, 2, 12, 12, 12, 12, 1,
                             12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 8, 8, 8
                            };

//Main menu melody
int noteDurations_menu[] = {8, 8, 8, 8, 8,
                            4, 4, 4, 4,
                            8, 8, 8, 8, 8, 8, 8
                           };

//Failed shot
int noteDurations_failed[] = {4, 2};

//Winner
int noteDurations_winner[] = {4, 4, 4, 4, 4, 4, 4, 4};

//Game over music
int noteDurations_over[] = {2, 2, 1};

//No more bullets
int noteDurations_bull[] = {1};
//*****************************************************************
//                DEFINE PINS FOR CONNECTIONS
//*****************************************************************
#define LCD_RST PD_0
#define LCD_CS PD_1
#define LCD_RS PD_2
#define LCD_WR PD_3
#define LCD_RD PE_1
int DPINS[] = {PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7};

//****************************************************************
//                     FUNCTIONS PROTOTYPES
//****************************************************************
void LCD_Init(void);
void LCD_CMD(uint8_t cmd);
void LCD_DATA(uint8_t data);
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void LCD_Clear(unsigned int c);
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void LCD_Print(String text, int x, int y, int fontSize, int color, int background);
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]);
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[], int columns, int index, char flip, char offset);
bool Collision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
int ascii_hex(int a);
void MapSD(char x[]);
void bird_movement(void);
//****************************************************************
//                   BITMAPS IMPORTED FROM FLASH GRAFICOS.C
//****************************************************************
extern uint8_t dogBitmap[]; // Dog Sprite
extern uint8_t bird1Bitmap[]; // Bird Sprite
extern uint8_t gunBitmap[]; //Gun image
extern uint8_t fondo[];
extern uint8_t background[];

//****************************************************************
//                   SD CONFIGURATION
//****************************************************************
Sd2Card card;
SdVolume volume;
SdFile root;
int lecturaserial;
const int chipSelect = PA_3; //cs PIN
File pic;

//****************************************************************
//                   DEFINING STRUCTS
//****************************************************************
struct Sprite { // Sprite Structure
  int x; // X position
  int y; // Y position
  int width; // bitmap width
  int height; // bitmap height
  int columns; // columna sprite sheet
  int index; // index sprite sheet
  int flip; // flip sprite
  int offset; // offset from origin
} dog, bird, gun; // Define the objects

struct Rectangle { // Rectangle Structrure
  int x; // X position
  int y; // Y position
  int width; // Width
  int height; // height
  int color; // Fill color
} rect, backg, menu, sky; // Define the rectangles used

struct Ground { //struct to draw a h_line
  int x;
  int y;
  int width;
  int height;
  int color;
} gnd;

//****************************************************************
//                   VARIABLES FOR CONDITIONS (FLAGS)
//****************************************************************
bool rectUp = false; // birdDirection
bool collision = false; // colision detection
unsigned long previousMillis = 0;
const long interval = 42;
int range;
int pos1, pos2, pos3, pos4, pos5, pos6, pos7, pos8, pos9, pos10, pos11, pos12;
bool avai;
int start, state1, state2, counter1, counter2, start1, already, previous;
int move1, move2;
char shot, shot1, score, score1, played, Pselected, endgame;
char fail_1, fail;
char endgame1;
//****************************************************************
//                            SETUP
//****************************************************************
void setup() {
  //Serial2 to receive from arduino and tiva
  //Serial3 to recieve from tiva to tiva
  Serial3.begin(9600);
  Serial3.setTimeout(50);
  delay(100);
  Serial2.begin(9600);
  Serial2.setTimeout(50);
  delay(100);
  SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
  Serial.begin(9600);
  GPIOPadConfigSet(GPIO_PORTB_BASE, 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);
  Serial.println("Start");
  LCD_Init();
  LCD_Clear(0x00);

  // SPI comunication for SD reading
  SPI.setModule(0);
  Serial.print("\nInitializing SD card...");
  pinMode(PA_3, OUTPUT);     // change this to 53 on a mega
  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card is inserted?");
    Serial.println("* Is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    return;
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  // print the type of card
  Serial.print("\nCard type: ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }
  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("\nVolume type is FAT");
  Serial.println(volume.fatType(), DEC);
  Serial.println();

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize *= 512;                            // SD card blocks are always 512 bytes
  Serial.print("Volume size (bytes): ");
  Serial.println(volumesize);
  Serial.print("Volume size (Kbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Mbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);


  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);

  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);

  pinMode(PUSH1, INPUT_PULLUP); // integrated buttons as pull ups
  pinMode(PUSH2, INPUT_PULLUP);
  pinMode(PE_3, INPUT_PULLUP);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  //Define dog sprite
  dog.x = 0;
  dog.y = 150;
  dog.width = 60;
  dog.height = 44;
  dog.columns = 4;
  dog.index = 0;
  dog.flip = 0;
  dog.offset = 0;
  // Define gun sprite
  gun.x = 90;
  gun.y = 200;
  gun.width = 29;
  gun.height = 30;
  gun.columns = 1;
  gun.index = 0;
  gun.flip = 0;
  gun.offset = 0;
  // Define bird first movement
  bird.x = 90;
  bird.y = 50;
  bird.width = 42;
  bird.height = 26;
  bird.columns = 2;
  bird.index = 0;
  bird.flip = 0;
  bird.offset = 0;
  //Define green as the grass
  rect.x = 0;
  rect.y = 150;
  rect.width = 320;
  rect.height = 90;
  rect.color = 0x27E0;
  //Define brown as the ground
  gnd.x = 0;
  gnd.y = 193;
  gnd.width = 320;
  gnd.height = 7;
  gnd.color = 0x59C0;

  avai = true;

  /////////////////////////////////////////////////////////////////////////////
  //                  Intro to the Game                                      //
  /////////////////////////////////////////////////////////////////////////////
  //LCD_Clear(0x00);
  FillRect(backg.x, backg.y = 0, backg.width = 320, backg.height = 240, backg.color = 0x655F);
  FillRect(rect.x, rect.y, rect.width, rect.height, rect.color);
  FillRect(gnd.x, gnd.y, gnd.width, gnd.height, gnd.color);
  LCD_Print("DUCK HUNT", 90, 50, 2, 0x00, 0x655F); //Text is then covered by Duck movement
  LCD_Print("by Marco Duarte", 100, 70, 1, 0x00, 0x655F); //Text is then covered by sky again
  LCD_Print("and Jonathan Pu", 100, 90, 1, 0x00, 0x655F);
  LCD_Print("2021", 150, 110, 1, 0x00, 0x655F);
  //if (Serial3.available()) {
  Serial3.write(49);
  //delay(4);
  //}
  //tone_function(melody_intro, noteDurations_intro, 26); in case it wants to be rep. from main tiva
  //Animate the dog before the game begins
  for (int x = 0; x < 320 - dog.width; x++) {
    delay(10);
    //LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[],int columns, int index, char flip, char offset);
    // Dog animation
    dog.index = (x / 11) % 8;
    LCD_Sprite(x, dog.y, dog.width, dog.height, dogBitmap, dog.columns, dog.index, dog.flip, dog.offset);
    V_line( x - 1, 150, 43, 0x27E0);
  }
  //Cover dog when is at the end
  FillRect(rect.x = 0, rect.y = 150, rect.width = 320, rect.height = 45, rect.color = 0x27E0);
  //INTRO
  //Cover letters with blue at the end of the intro
  for (int x = 0; x < 320; x += 10) {
    delay(10);
    FillRect(x, backg.y, backg.width = 100, backg.height = 120, 0x655F);
    //V_line( x - 1, 100, 43, 0x655F);
  }
  start = 1;
  start1 = 0;
}

//****************************************************************
//                           MAIN LOOP
//****************************************************************
void loop() {

  unsigned long currentMillis = millis();
  // frame update every 42ms = 24fps
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    //Now we need to display the main menu in which Players can choose who's going to start
    //If player 1 is selected first, at the end, only P2 can be next, and the other way around
    if (start == 1) {
      //if (Serial3.available()) {
      Serial3.write(50); //Intro music
      //delay(4);
      //}
      //Display main menu
      FillRect(menu.x = 0, menu.y = 0, menu.width = 320, menu.height = 240, 0x00);
      LCD_Print("Select who is", 50, 50, 2, 0xFFFF, 0x00);
      LCD_Print("going to start", 40, 70, 2, 0xFFFF, 0x00);
      LCD_Print("Player 1", 90, 120, 2, 0xFFE0, 0x00);
      LCD_Print("Player 2", 90, 160, 2, 0xFFE0, 0x00);

      start = 0;
    }

    //FUNCTIONS TO CHANGE BETWEEN P1 AND P2
    if ((digitalRead(PUSH1) == LOW) and start1 == 0 and already == 0) {
      Rect(80, 115, 150, 30, 0xF800);
      Rect(80, 155, 150, 30, 0x00);
      start1 = 0;
      already = 0; //no choice made yet
      previous = 1;
    }
    if ((digitalRead(PUSH2) == LOW) and start1 == 0 and already == 0) {
      Rect(80, 155, 150, 30, 0xF800);
      Rect(80, 115, 150, 30, 0x00);
      start1 = 0;
      already = 0; //no choice made yet
      previous = 2;
    }
    //FINALLY LET'S MAKE A CHOICE USING THE previous FLAG and Third button
    if ((digitalRead(PE_3) == LOW) and previous == 1) {
      digitalWrite(BLUE_LED, 1);
      digitalWrite(RED_LED, 0);
      //already = 1;
      Pselected = 1;
      //played = 1;
      start1 = 1; //P1 was selected, this will help for the next round
    }
    else if ((digitalRead(PE_3) == LOW) and previous == 2) {
      digitalWrite(BLUE_LED, 0);
      digitalWrite(RED_LED, 1);
      //already = 1;
      Pselected = 2;
      //played = 2;
      start1 = 2; //P2 was selected, this will help for the next round
    }
    //////////////////////////////////////////////////////////////////////////
    //                        Game Running                                  //
    //////////////////////////////////////////////////////////////////////////
    if (start1 == 1) {
      //Set stage for the game
      FillRect(backg.x, backg.y = 0, backg.width = 320, backg.height = 240, backg.color = 0x655F);
      FillRect(rect.x, rect.y, rect.width, rect.height = 200, rect.color);
      FillRect(gnd.x, gnd.y, gnd.width, gnd.height, gnd.color);
      LCD_Print("Player 1", 135, 10, 1, 0xFFFF, 0x0000);
      LCD_Print("Score", 20, 20, 1, 0x00, 0x655F);
      LCD_Print("000", 20, 30, 1, 0x00, 0x655F);
      start1 = 0;
      already = 1;
      //played = 1; //Indicates this one was chosen first
      digitalWrite(BLUE_LED, 0);
    }
    /////////////////////////////PLAYER1/////////////////////////////////////
    if (already == 1) {
      bird_movement(); //function of bird sprite
      if (Serial3.available()) {
        move1 = Serial3.read();
        Serial.write(move1);
        delay(4);
        //GUN DEFINITION
        //Define sprite for the gun
        LCD_Sprite(gun.x, gun.y, gun.width, gun.height, gunBitmap, gun.columns, gun.index, 0, gun.offset);
        //Movement of gun to the right and left with PB
        if (move1 == 49) { // sprite modification
          gun.x += 2;
          //defining ranges and variables to set the bird and gun in a certain position
          if (gun.x > 90 and gun.x <= 150) {
            range = 1;
            pos1 = 1;
            pos2 = 0;
            pos3 = 0;
            pos7 = 1;
          };
          if (gun.x > 151 and gun.x <= 210) {
            range = 2;
            pos1 = 2;
            pos2 = 1;
            pos3 = 0;
            pos7 = 2;
          };
          if (gun.x > 211 and gun.x <= 270) {
            range = 3;
            pos1 = 3;
            pos2 = 0;
            pos3 = 1;
            pos7 = 3;
          };

          if (gun.x == (320 - 50)) {
            gun.x = (320 - 50);

            if (move1 == 49) {
              gun.x -= 2;
              move1 = 0;
            }
          }
        }
        //movement to the other side
        if (move1 == 51) {
          gun.x -= 2;
          if (gun.x > 90 and gun.x <= 150) {
            range = 1;
            pos1 = 1;
            pos2 = 0;
            pos3 = 0;
            pos7 = 1;
          };
          if (gun.x > 151 and gun.x <= 210) {
            range = 2;
            pos1 = 2;
            pos2 = 1;
            pos3 = 0;
            pos7 = 2;
          };
          if (gun.x > 211 and gun.x <= 270) {
            range = 3;
            pos1 = 3;
            pos2 = 0;
            pos3 = 1;
            pos7 = 3;
          };

          if (gun.x == (90)) {
            gun.x = 90;

            if (move1 == 51) {
              gun.x += 2;
              move1 = 0;
            }
          }
        }
        if (gun.flip == 1) { // dependiendo de la dirección, se colorea resto del sprite del frame anterior
          FillRect(gun.x - 4, gun.y, 4, gun.height, 0x27E0);
        }
        else {
          FillRect(gun.x + gun.width, gun.y, 4, gun.height, 0x27E0);
        }
        //shots received from the tiva and arduino controller
        if (move1 == 50) {
          shot = 0;
          if (pos4 == 1 and pos1 == 1) {
            //shot = 0;
            score++;
            endgame = 1;
          }
          else if (pos4 == 2 and pos1 == 2) {
            //shot = 0;
            score++;
            endgame = 1;
          }
          else if (pos4 == 3 and pos1 == 3) {
            //shot = 0;
            score++;
            endgame = 1;
          }
          else {
            score = score;
            Serial3.write(51); //missed
            endgame = endgame;
          }
        }
        if (move1 == 52) {
          shot = 0;
          if (pos4 == 1 and pos1 == 1) {
            //shot = 0;
            score++;
            endgame = 2;
          }
          else if (pos4 == 2 and pos1 == 2) {
            //shot = 0;
            score++;
          }
          else if (pos4 == 3 and pos1 == 3) {
            //shot = 0;
            score++;
            endgame = 2;
          }
          else {
            score = score;
            Serial3.write(51); //missed
            endgame = endgame;
          }
        }
        if (move1 == 57) {
          shot = 1; //variable to reinitalize controller (bullets are over)
          if (pos4 == 1 and pos1 == 1) {
            score++;
            endgame = 3;
          }
          else if (pos4 == 2 and pos1 == 2) {
            score++;
            endgame = 3;
          }
          else if (pos4 == 3 and pos1 == 3) {
            score++;
            endgame = 3;
          }
          else {
            score = score;
            Serial3.write(51);
            endgame = endgame;
          }
        }
        //What is the current score
        if (endgame == 1) {
          digitalWrite(GREEN_LED, 1);
          digitalWrite(RED_LED, 0);
          digitalWrite(BLUE_LED, 0);
          LCD_Print("050", 20, 30, 1, 0x00, 0x655F);
          FillRect(70, 50, 270, 70, 0x655F);
          LCD_Print("Nice shot!", 90, 50, 2, 0x00, 0x655F);
          delay(500);
          endgame = 0; //so we can move gun again
          bird_movement();
        }
        else if (endgame == 2) {
          digitalWrite(GREEN_LED, 0);
          digitalWrite(RED_LED, 1);
          digitalWrite(BLUE_LED, 0);
          LCD_Print("100", 20, 30, 1, 0x00, 0x655F);
          FillRect(70, 50, 270, 70, 0x655F);
          LCD_Print("Smooth!", 90, 50, 2, 0x00, 0x655F);
          delay(500);
          endgame = 0;
          bird_movement();

        }
        else if (endgame == 3) {
          digitalWrite(GREEN_LED, 0);
          digitalWrite(RED_LED, 0);
          digitalWrite(BLUE_LED, 1);
          LCD_Print("150", 20, 30, 1, 0x00, 0x655F);
          FillRect(70, 50, 270, 70, 0x655F);
          LCD_Print("Terrific!", 90, 50, 2, 0x00, 0x655F);
          delay(500);
          endgame = 0;
          bird_movement();

        }
        else {
          digitalWrite(GREEN_LED, 0);
          digitalWrite(RED_LED, 0);
          digitalWrite(BLUE_LED, 0);
        }
        //Bullets are over
        if (shot == 1) {
          Serial3.write(52); //no more bullets
          delay(1000);
          Serial3.write(60); //reset controller
          //Player 1 already played
          FillRect(90, 50, 150, 60, 0x655F);
          Serial3.write(54);
          LCD_Print("GAME", 135, 50, 2, 0xFFFF, 0x655F);
          LCD_Print("OVER", 135, 70, 2, 0xFFFF, 0x655F);
          delay(1000);
          //Pselected = 1;
          if (Pselected == 2) {
            start1 = 0;
            already = 0;
            winner();
          }
          else if (Pselected == 1) {
            start1 = 2;
          }
        }
      }
    }
    /////////////////////////////PLAYER2/////////////////////////////////////
    if (start1 == 2) {
      FillRect(backg.x, backg.y = 0, backg.width = 320, backg.height = 240, backg.color = 0x655F);
      FillRect(rect.x, rect.y, rect.width, rect.height = 200, rect.color);
      FillRect(gnd.x, gnd.y, gnd.width, gnd.height, gnd.color);
      LCD_Print("Player 2", 135, 10, 1, 0xFFFF, 0x0000);
      LCD_Print("Score", 20, 20, 1, 0x00, 0x655F);
      LCD_Print("000", 20, 30, 1, 0x00, 0x655F);
      start1 = 0;
      already = 2;
      //played = 2;
      digitalWrite(BLUE_LED, 0);
    }

    if (already == 2) {
      bird_movement();

      if (Serial2.available()) {
        move2 = Serial2.read();
        Serial.write(move2);
        delay(4);
        //shot = Serial3.read();
        //GUN DEFINITION
        //Define sprite for the gun
        LCD_Sprite(gun.x, gun.y, gun.width, gun.height, gunBitmap, gun.columns, gun.index, 0, gun.offset);
        //Movement of gun to the right and left with PB
        if (move2 == 69) { // modificación de atributos de sprite
          gun.x += 2;
          if (gun.x > 90 and gun.x <= 150) {
            range = 1;
            pos1 = 1;
            pos2 = 0;
            pos3 = 0;
            pos7 = 1;
          };
          if (gun.x > 151 and gun.x <= 210) {
            range = 2;
            pos1 = 2;
            pos2 = 1;
            pos3 = 0;
            pos7 = 2;
          };
          if (gun.x > 211 and gun.x <= 270) {
            range = 3;
            pos1 = 3;
            pos2 = 0;
            pos3 = 1;
            pos7 = 3;
          };

          if (gun.x == (320 - 50)) {
            gun.x = (320 - 50);

            if (move2 == 69) {
              gun.x -= 2;
              move2 = 0;
            }
          }
        }
        if (move2 == 70) {
          gun.x -= 2;
          if (gun.x > 90 and gun.x <= 150) {
            range = 1;
            pos1 = 1;
            pos2 = 0;
            pos3 = 0;
            pos7 = 1;
          };
          if (gun.x > 151 and gun.x <= 210) {
            range = 2;
            pos1 = 2;
            pos2 = 1;
            pos3 = 0;
            pos7 = 2;
          };
          if (gun.x > 211 and gun.x <= 270) {
            range = 3;
            pos1 = 3;
            pos2 = 0;
            pos3 = 1;
            pos7 = 3;
          };

          if (gun.x == (90)) {
            gun.x = 90;

            if (move2 == 70) {
              gun.x += 2;
              move2 = 0;
            }
          }
        }
        if (gun.flip == 1) { // dependiendo de la dirección, se colorea resto del sprite del frame anterior
          FillRect(gun.x - 4, gun.y, 4, gun.height, 0x27E0);
        }
        else {
          FillRect(gun.x + gun.width, gun.y, 4, gun.height, 0x27E0);
        }
        if (move2 == 65) {
          shot1 = 0;
          if (pos4 == 1 and pos1 == 1) {
            //shot1 = 0;
            score1++;
            endgame1 = 1;
            //fail_1 = 0;
          }
          else if (pos4 == 2 and pos1 == 2) {
            //shot1 = 0;
            score1++;
            endgame1 = 1;
            //fail_1 = 0;
          }
          else if (pos4 == 3 and pos1 == 3) {
            //shot1 = 0;
            score1++;
            endgame1 = 1;
            //fail_1 = 0;
          }
          else {
            score1 = score1;
            Serial3.write(51); //missed
            endgame1 = endgame1;
            //fail_1 = 1;
          }
        }
        if (move2 == 66) {
          shot1 = 0;
          if (pos4 == 1 and pos1 == 1) {
            //shot1 = 0;
            score1++;
            endgame1 = 2;
            //fail_1 = 0;
          }
          else if (pos4 == 2 and pos1 == 2) {
            //shot1 = 0;
            score1++;
            endgame1 = 2;
            //fail_1 = 0;
          }
          else if (pos4 == 3 and pos1 == 3) {
            //shot1 = 0;
            score1++;
            endgame1 = 2;
            //fail_1 = 0;
          }
          else {
            score1 = score1;
            Serial3.write(51); //missed
            endgame1 = endgame1;
            //fail_1 = 1;
          }
        }
        if (move2 == 67) {
          shot1 = 1; //variable to reinitalize controller (bullets are over)
          if (pos4 == 1 and pos1 == 1) {
            score1++;
            endgame = 3;
            //fail_1 = 0;
          }
          else if (pos4 == 2 and pos1 == 2) {
            score1++;
            endgame = 3;
            //fail_1 = 0;
          }
          else if (pos4 == 3 and pos1 == 3) {
            score1++;
            endgame = 3;
            //fail_1 = 0;
          }
          else {
            score1 = score1;
            Serial3.write(51); //missed
            endgame1 = endgame1;
          }
        }
        if (endgame1 == 1 ) {
          digitalWrite(GREEN_LED, 1);
          digitalWrite(RED_LED, 0);
          digitalWrite(BLUE_LED, 0);
          LCD_Print("050", 20, 30, 1, 0x00, 0x655F);
          FillRect(70, 50, 270, 70, 0x655F);
          LCD_Print("Nice shot!", 90, 50, 2, 0x00, 0x655F);
          delay(500);
          endgame1 = 0;
          bird_movement();
        }
        else if (endgame1 == 2) {
          digitalWrite(GREEN_LED, 0);
          digitalWrite(RED_LED, 1);
          digitalWrite(BLUE_LED, 0);
          LCD_Print("100", 20, 30, 1, 0x00, 0x655F);
          FillRect(70, 50, 270, 70, 0x655F);
          LCD_Print("Great Kill!", 90, 50, 2, 0x00, 0x655F);
          delay(500);
          endgame1 = 0;
          bird_movement();
        }
        else if (endgame1 == 3) {
          digitalWrite(GREEN_LED, 0);
          digitalWrite(RED_LED, 0);
          digitalWrite(BLUE_LED, 1);
          LCD_Print("150", 20, 30, 1, 0x00, 0x655F);
          FillRect(70, 50, 270, 70, 0x655F);
          LCD_Print("Terrific!", 90, 50, 2, 0x00, 0x655F);
          delay(500);
          endgame1 = 0;
          bird_movement();

        }
        else {
          digitalWrite(GREEN_LED, 0);
          digitalWrite(RED_LED, 0);
          digitalWrite(BLUE_LED, 0);
        }
        //Bullets are over
        if (shot1 == 1) {
          delay(1000);
          Serial2.write(68);

          //Player 2 already played
          FillRect(90, 50, 150, 60, 0x655F);
          Serial3.write(54);
          LCD_Print("GAME", 135, 50, 2, 0xFFFF, 0x655F);
          LCD_Print("OVER", 135, 70, 2, 0xFFFF, 0x655F);
          if (Pselected == 1) {
            already = 0;
            winner();
          }
          else if (Pselected == 2) {
            start1 = 1;
          }
        }
      }
    }
  }
}

void winner(void) {
  //COMPARING SCORES
  if (score > score1) {
    LCD_Print("P1 wins", 135, 200, 2, 0x00, 0xFFFF);
    Serial3.write(53); //winner music
    MapSD("win1.txt");
  }
  else if (score1 > score) {
    LCD_Print("P2 wins", 135, 200, 2, 0x00, 0xFFFF);
    Serial3.write(53); //winner music
    MapSD("win2.txt");
  }
  else {
    LCD_Print("DRAW", 135, 200, 2, 0x00, 0xFFFF);
    Serial3.write(53); //winner music
    MapSD("main.txt");
  }
}
//**************************************************************
//                 Función para inicializar LCD
//**************************************************************
void LCD_Init(void) {
  pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_WR, OUTPUT);
  pinMode(LCD_RD, OUTPUT);
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(DPINS[i], OUTPUT);
  }
  //***********************************************************
  // Secuencia de Inicialización
  //***********************************************************
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_WR, HIGH);
  digitalWrite(LCD_RD, HIGH);
  digitalWrite(LCD_RST, HIGH);
  delay(5);
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);
  digitalWrite(LCD_CS, LOW);
  //******
  LCD_CMD(0xE9);  // SETPANELRELATED
  LCD_DATA(0x20);
  //******
  LCD_CMD(0x11); // Exit Sleep SLEEP OUT (SLPOUT)
  delay(100);
  //******
  LCD_CMD(0xD1);    // (SETVCOM)
  LCD_DATA(0x00);
  LCD_DATA(0x71);
  LCD_DATA(0x19);
  //******
  LCD_CMD(0xD0);   // (SETPOWER)
  LCD_DATA(0x07);
  LCD_DATA(0x01);
  LCD_DATA(0x08);
  //******
  LCD_CMD(0x36);  // (MEMORYACCESS)
  LCD_DATA(0x40 | 0x80 | 0x20 | 0x08); // LCD_DATA(0x19);
  //******
  LCD_CMD(0x3A); // Set_pixel_format (PIXELFORMAT)
  LCD_DATA(0x05); // color setings, 05h - 16bit pixel, 11h - 3bit pixel
  //******
  LCD_CMD(0xC1);    // (POWERCONTROL2)
  LCD_DATA(0x10);
  LCD_DATA(0x10);
  LCD_DATA(0x02);
  LCD_DATA(0x02);
  //******
  LCD_CMD(0xC0); // Set Default Gamma (POWERCONTROL1)
  LCD_DATA(0x00);
  LCD_DATA(0x35);
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x02);
  //******
  LCD_CMD(0xC5); // Set Frame Rate (VCOMCONTROL1)
  LCD_DATA(0x04); // 72Hz
  //******
  LCD_CMD(0xD2); // Power Settings  (SETPWRNORMAL)
  LCD_DATA(0x01);
  LCD_DATA(0x44);
  //******
  LCD_CMD(0xC8); //Set Gamma  (GAMMASET)
  LCD_DATA(0x04);
  LCD_DATA(0x67);
  LCD_DATA(0x35);
  LCD_DATA(0x04);
  LCD_DATA(0x08);
  LCD_DATA(0x06);
  LCD_DATA(0x24);
  LCD_DATA(0x01);
  LCD_DATA(0x37);
  LCD_DATA(0x40);
  LCD_DATA(0x03);
  LCD_DATA(0x10);
  LCD_DATA(0x08);
  LCD_DATA(0x80);
  LCD_DATA(0x00);
  //******
  LCD_CMD(0x2A); // Set_column_address 320px (CASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x3F);
  //******
  LCD_CMD(0x2B); // Set_page_address 480px (PASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0xE0);
  //  LCD_DATA(0x8F);
  LCD_CMD(0x29); //display on
  LCD_CMD(0x2C); //display on

  LCD_CMD(ILI9341_INVOFF); //Invert Off
  delay(120);
  LCD_CMD(ILI9341_SLPOUT);    //Exit Sleep
  delay(120);
  LCD_CMD(ILI9341_DISPON);    //Display on
  digitalWrite(LCD_CS, HIGH);
}
//*******************************************************************
// Función para enviar comandos a la LCD - parámetro (comando)
//*******************************************************************
void LCD_CMD(uint8_t cmd) {
  digitalWrite(LCD_RS, LOW);
  digitalWrite(LCD_WR, LOW);
  GPIO_PORTB_DATA_R = cmd;
  digitalWrite(LCD_WR, HIGH);
}
//******************************************************************
// Función para enviar datos a la LCD - parámetro (dato)
//******************************************************************
void LCD_DATA(uint8_t data) {
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_WR, LOW);
  GPIO_PORTB_DATA_R = data;
  digitalWrite(LCD_WR, HIGH);
}
//******************************************************************
// Función para definir rango de direcciones de memoria con las cuales se trabajara (se define una ventana)
//******************************************************************
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) {
  LCD_CMD(0x2a); // Set_column_address 4 parameters
  LCD_DATA(x1 >> 8);
  LCD_DATA(x1);
  LCD_DATA(x2 >> 8);
  LCD_DATA(x2);
  LCD_CMD(0x2b); // Set_page_address 4 parameters
  LCD_DATA(y1 >> 8);
  LCD_DATA(y1);
  LCD_DATA(y2 >> 8);
  LCD_DATA(y2);
  LCD_CMD(0x2c); // Write_memory_start
}
//**********************************************************
// Función para borrar la pantalla - parámetros (color)
//**********************************************************
void LCD_Clear(unsigned int c) {
  unsigned int x, y;
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  SetWindows(0, 0, 319, 239); // 479, 319);
  for (x = 0; x < 320; x++)
    for (y = 0; y < 240; y++) {
      LCD_DATA(c >> 8);
      LCD_DATA(c);
    }
  digitalWrite(LCD_CS, HIGH);
}
//*************************************************************
// Función para dibujar una línea horizontal - parámetros ( coordenada x, cordenada y, longitud, color)
//*************************************************************
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
  unsigned int i, j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + x;
  SetWindows(x, y, l, y);
  j = l;// * 2;
  for (i = 0; i < l; i++) {
    LCD_DATA(c >> 8);
    LCD_DATA(c);
  }
  digitalWrite(LCD_CS, HIGH);
}
//**************************************************************
// Función para dibujar una línea vertical - parámetros ( coordenada x, cordenada y, longitud, color)
//**************************************************************
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
  unsigned int i, j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + y;
  SetWindows(x, y, x, l);
  j = l; //* 2;
  for (i = 1; i <= j; i++) {
    LCD_DATA(c >> 8);
    LCD_DATA(c);
  }
  digitalWrite(LCD_CS, HIGH);
}
//**************************************************************
// Función para dibujar un rectángulo - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//**************************************************************
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  H_line(x  , y  , w, c);
  H_line(x  , y + h, w, c);
  V_line(x  , y  , h, c);
  V_line(x + w, y  , h, c);
}
//***************************************************************
// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);

  unsigned int x2, y2;
  x2 = x + w;
  y2 = y + h;
  SetWindows(x, y, x2 - 1, y2 - 1);
  unsigned int k = w * h * 2 - 1;
  unsigned int i, j;
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      LCD_DATA(c >> 8);
      LCD_DATA(c);
      k = k - 2;
    }
  }
  digitalWrite(LCD_CS, HIGH);
}
//**************************************************************
// Función para dibujar texto - parámetros ( texto, coordenada x, cordenada y, color, background)
//**************************************************************
void LCD_Print(String text, int x, int y, int fontSize, int color, int background) {
  int fontXSize ;
  int fontYSize ;

  if (fontSize == 1) {
    fontXSize = fontXSizeSmal ;
    fontYSize = fontYSizeSmal ;
  }
  if (fontSize == 2) {
    fontXSize = fontXSizeBig ;
    fontYSize = fontYSizeBig ;
  }

  char charInput ;
  int cLength = text.length();
  Serial.println(cLength, DEC);
  int charDec ;
  int c ;
  int charHex ;
  char char_array[cLength + 1];
  text.toCharArray(char_array, cLength + 1) ;
  for (int i = 0; i < cLength ; i++) {
    charInput = char_array[i];
    Serial.println(char_array[i]);
    charDec = int(charInput);
    digitalWrite(LCD_CS, LOW);
    SetWindows(x + (i * fontXSize), y, x + (i * fontXSize) + fontXSize - 1, y + fontYSize );
    long charHex1 ;
    for ( int n = 0 ; n < fontYSize ; n++ ) {
      if (fontSize == 1) {
        charHex1 = pgm_read_word_near(smallFont + ((charDec - 32) * fontYSize) + n);
      }
      if (fontSize == 2) {
        charHex1 = pgm_read_word_near(bigFont + ((charDec - 32) * fontYSize) + n);
      }
      for (int t = 1; t < fontXSize + 1 ; t++) {
        if (( charHex1 & (1 << (fontXSize - t))) > 0 ) {
          c = color ;
        } else {
          c = background ;
        }
        LCD_DATA(c >> 8);
        LCD_DATA(c);
      }
    }
    digitalWrite(LCD_CS, HIGH);
  }
}
//****************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
//****************************************************************
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]) {
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);

  unsigned int x2, y2;
  x2 = x + width;
  y2 = y + height;
  SetWindows(x, y, x2 - 1, y2 - 1);
  unsigned int k = 0;
  unsigned int i, j;

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k + 1]);
      //LCD_DATA(bitmap[k]);
      k = k + 2;
    }
  }
  digitalWrite(LCD_CS, HIGH);
}
//****************************************************************
// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
//****************************************************************
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[], int columns, int index, char flip, char offset) {
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);

  unsigned int x2, y2;
  x2 =   x + width;
  y2 =    y + height;
  SetWindows(x, y, x2 - 1, y2 - 1);
  int k = 0;
  int ancho = ((width * columns));
  if (flip) {
    for (int j = 0; j < height; j++) {
      k = (j * (ancho) + index * width - 1 - offset) * 2;
      k = k + width * 2;
      for (int i = 0; i < width; i++) {
        LCD_DATA(bitmap[k]);
        LCD_DATA(bitmap[k + 1]);
        k = k - 2;
      }
    }
  }
  else {
    for (int j = 0; j < height; j++) {
      k = (j * (ancho) + index * width + 1 + offset) * 2;
      for (int i = 0; i < width; i++) {
        LCD_DATA(bitmap[k]);
        LCD_DATA(bitmap[k + 1]);
        k = k + 2;
      }
    }
  }
  digitalWrite(LCD_CS, HIGH);
}
bool Collision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
  return (x1 < x2 + w2) && (x1 + w1 > x2) && (y1 < y2 + h2) && (y1 + h1 > y2);
}

//**************************************************************
//            FUNCTION TO CONVERT FROM HEX TO ASCII
//**************************************************************
int ascii_hex(int a) {
  switch (a) {
    case 48:
      return 0;
    case 49:
      return 1;
    case 50:
      return 2;
    case 51:
      return 3;
    case 52:
      return 4;
    case 53:
      return 5;
    case 54:
      return 6;
    case 55:
      return 7;
    case 56:
      return 8;
    case 57:
      return 9;
    case 97:
      return 10;
    case 98:
      return 11;
    case 99:
      return 12;
    case 100:
      return 13;
    case 101:
      return 14;
    case 102:
      return 15;
  }
}

//Function to map a txt from SD to display in the LCD
void MapSD(char x[]) {
  pic = SD.open(x, FILE_READ);
  int hex1 = 0;
  int val1 = 0;
  int val2 = 0;
  int mapear = 0;
  int vert = 0;

  unsigned char maps[640];
  if (pic) {
    Serial.println("Reading file...");
    while (pic.available()) {
      mapear = 0;
      while (mapear < 640) {
        hex1 = pic.read();
        if (hex1 == 120) {
          val1 = pic.read();
          val2 = pic.read();
          val1 = ascii_hex(val1);
          val2 = ascii_hex(val2);
          maps[mapear] = val1 * 16 + val2;
          mapear++;
        }
      }
      LCD_Bitmap(0, vert, 320, 1, maps);
      vert++;
    }
    pic.close();
  }
  else {
    Serial.println("File couldn't be read...");
    pic.close();
  }
}

//Movement of birt sprite
void bird_movement(void) {
  //Movement of the bird to the left and the right without a for loop
  if (rectUp) { // Rectangle movement
    //digitalWrite(RED_LED, 1);
    LCD_Sprite(bird.x , bird.y, bird.width, bird.height, bird1Bitmap, bird.columns, bird.index, 1, 0); // se colorea resto de rectángulo del frame anterior
    bird.x -= 5;
    // Bird range
    if (bird.x > 90 and bird.x <= 150) {
      range = 1;
      pos4 = 1; // Flag to indicate the range of the bird
      pos5 = 0;
      pos6 = 0;
      pos8 = 1;
    };
    if (bird.x > 151 and bird.x <= 210) {
      range = 2;
      pos4 = 2;
      pos5 = 1;
      pos6 = 0;
      pos8 = 2;
    };
    if (bird.x > 211 and bird.x <= 270) {
      range = 3;
      pos4 = 3;
      pos5 = 0;
      pos6 = 1;
      pos8 = 3;
    };
    bird.flip = 0;
    if (bird.x <= 90) {
      rectUp = false;
    }
  }
  else {
    LCD_Sprite(bird.x , bird.y, bird.width, bird.height, bird1Bitmap, bird.columns, bird.index, 0, 0);
    bird.x += 5;
    if (bird.x > 90 and bird.x <= 150) {
      range = 1;
      pos4 = 1;
      pos5 = 0;
      pos6 = 0;
      pos8 = 1;
    };
    if (bird.x > 151 and bird.x <= 210) {
      range = 2;
      pos4 = 2;
      pos5 = 1;
      pos6 = 0;
      pos8 = 2;
    };
    if (bird.x > 211 and bird.x <= 270) {
      range = 3;
      pos4 = 3;
      pos5 = 0;
      pos6 = 1;
      pos8 = 3;
    };
    bird.flip = 1;
    if (bird.x >= 280) {
      rectUp = true;
    }
  }
  if (bird.flip == 1) { // dependiendo de la dirección, se colorea resto del sprite del frame anterior
    FillRect(bird.x - 4, bird.y, 4, bird.height, 0x655F);
  }
  else {
    FillRect(bird.x + bird.width, bird.y, 4, bird.height, 0x655F);
  }
}
//In case music wants to be implemented by this main tiva before or after a certain action
void tone_function(int mel[], int duration[], int quant) {
  //int sizeMel = sizeof mel/sizeof mel[0];
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 30; thisNote++) {
    // to calculate the note duration, take one second
    // divided by the note type. //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int durations = 1000 / duration[thisNote];
    tone(PF_4, mel[thisNote], durations); //important to use PF_4 'cause of the PWM signal
    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = durations * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(PF_4);
    if (thisNote == (quant - 1)) {
      break;
    }
  }
}
