/*
  MelodyPlays a melodycircuit:
   8-ohm speaker on digital pin 8
  created 21 Jan 2010
  modified 30 Aug 2011
  by Tom Igoe
  modified 7 Feb 2014
  by Mark Easley
  This example code is in the public domain.
  Modified code to include more sounds for the passive buzzer and Tiva-c
*/

//Library that contains frequencies for every defined note
#include "pitches.h"

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

//***********************************************************************
//            Variables for the controller of the game Buttons and leds
//***********************************************************************
#define BUTTON1 PB_5
#define BUTTON2 PB_0
#define BUTTON3 PB_1

const int LED3 = PE_5;
const int LED2 = PE_4;
const int LED1 = PB_4;
const int LED4 = PA_5;

int b1_state = 0;
int b2_state = 0;
int b3_state = 0;
int cont1 = 0;
int cont2 = 0;
int cont3 = 0;
int p1_flag, d1_flag, check1;
int p2_flag, d2_flag, check2;
int p3_flag, d3_flag, check3;
int shot_1, shot_2, shot_3;
char move1, move2;
char done;

//*************************************************************
//            Prototypes
//*************************************************************
void debounce1(void);

//*************************************************************
//            Main Software
//*************************************************************
void setup()
{
  Serial.begin(9600);
  Serial3.begin(9600);
  Serial3.setTimeout(50);
  delay(100);

  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
}

void loop()
{
  delay(10);
  b1_state = digitalRead(BUTTON1);
  debounce1();
  //If counter is 0 it will begin or reset the leds
  if (cont1 == 0) {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, LOW);
    shot_1 = 0;
    shot_2 = 0;
    shot_3 = 0;
  }
  //If counter is 1 it means it will send a bullet in the game
  if (cont1 == 1) {

    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, LOW);
    shot_1 = 0;
    shot_2 = 0;
    shot_3 = 1;
    Serial3.write(50);
    cont1 = 2; //Change counter so it doesn't loop
    
  }
  if (cont1 == 3) {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    shot_1 = 0;
    shot_2 = 1;
    shot_3 = 1;
    Serial3.write(52);
    cont1 = 4;
  }
  if (cont1 == 5) {
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, HIGH);
    shot_1 = 1;
    shot_2 = 1;
    shot_3 = 1;
    Serial3.write(57);
    cont1 = 6;
  }
  //This part will receive the reset signal from the main game
  if (Serial3.available()) {
    done = Serial3.read();
    Serial.write(done);
    //This will reset the controller
    if (done == 60) {
      delay(4);
      cont1 = 0;
    }
    //The other serial receives are to reproduce the melodies
    //at the same time that the LCD is showing pictures
    else if (done == 49) {
      delay(4); //intro music
      tone_function(melody_intro, noteDurations_intro, 26);
    }
    else if (done == 50) {
      delay(4); //menu music
      tone_function(melody_menu, noteDurations_menu, 16);
    }
    else if (done == 51) {
      delay(4); //missed
      tone_function(melody_failed, noteDurations_failed, 3);
    }
    else if (done == 52) {
      delay(4); //no bullets
      tone_function(melody_bull, noteDurations_bull, 1);
    }
    else if (done == 53) {
      delay(4); //winner
      tone_function(melody_winner, noteDurations_winner, 8);
    }
    else if (done == 54) {
      delay(4); //game over
      tone_function(melody_over, noteDurations_over, 3);
    }
  }
  if (digitalRead(BUTTON2) == HIGH) {

    Serial3.write(49);
  }
  // Controllers (push to right and left)
  if (digitalRead(BUTTON3) == HIGH) {
    Serial3.write(51);
  }
}

//****************************************************
//        Functions
//****************************************************

void debounce1(void) {
  if (b1_state == LOW)     //Asks if the push is pressed
  {
    d1_flag = 1;     //Variable changes value
  }
  if (b1_state == HIGH and d1_flag == 1)
  {

    //Makes the desired action
    d1_flag = 0;    //Variable turns to the original value
    cont1++;
  }
}


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
