//***********************************************************************
//            Variables for the controller of the game Buttons and leds
//***********************************************************************
#define BUTTON1 2
#define BUTTON2 12
#define BUTTON3 13

const int LED1 = 3;
const int LED2 = 4;
const int LED3 = 6;
const int LED4 = 8;

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
  Serial1.begin(9600);
  Serial1.setTimeout(50);
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
    Serial1.write(65);
    cont1 = 2; //Change counter so it doesn't loop
  }
  if (cont1 == 3) {

    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    shot_1 = 0;
    shot_2 = 1;
    shot_3 = 1;
    Serial1.write(66);
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
    Serial1.write(67);
    cont1 = 6;
  }
  //This part will receive the reset signal from the main game
  if (Serial1.available()) {
    done = Serial1.read();
    Serial1.write(done);
    //This will reset the controller
    if (done == 68) {
      delay(4);
      cont1 = 0;
    }
  }
  if (digitalRead(BUTTON2) == HIGH) {
    Serial1.write(69);
  }
  if (digitalRead(BUTTON3) == HIGH) {
    Serial1.write(70);
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
