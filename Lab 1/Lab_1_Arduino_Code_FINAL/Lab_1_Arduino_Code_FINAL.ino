// Global Variables
// PINS
const int RED_LED = 2;
const int YELLOW_LED = 3;
const int GREEN_LED = 4;
const int BUZZER = 5;
const int BUTTON = 18;
const int DATA = 8;
const int CLCK = 6;
const int LATCH = 7;
const int DIGIT_PINS[4] = {9, 10, 11, 12};

// TIMER CONSTANTS
const int TIMER_FSM = 200; // ms
const int TIMER_DISPLAY = 5; // ms
const int TIMER_DEBOUNCE = 15; // ms
const int FREQ_FSM = 1000/TIMER_FSM; //hz

// STATE VARIABLES
volatile unsigned int state = 0;
volatile unsigned int counter = 0;
volatile unsigned int timerLimit = 0; // s

// Hexadecimal 7-segment bit masks
byte seven_seg_digits[16] = { 
  B11111100, // = 0
  B01100000, // = 1 
  B11011010, // = 2
  B11110010, // = 3
  B01100110, // = 4
  B10110110, // = 5
  B10111110, // = 6
  B11100000, // = 7
  B11111110, // = 8
  B11100110, // = 9
  B11101110, // A
  B00111110, // b
  B10011100, // C
  B01111010, // d
  B10011110, // E
  B10001110, // F
};

void setup() {

  // Pin setup LED, Button, Buzzer
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  // Pin setup 4 digit 7-seg display
  pinMode(DATA, OUTPUT);
  pinMode(CLCK, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(DIGIT_PINS[0], OUTPUT);
  pinMode(DIGIT_PINS[1], OUTPUT);
  pinMode(DIGIT_PINS[2], OUTPUT);
  pinMode(DIGIT_PINS[3], OUTPUT);
  
  // Initialize LED values
  digitalWrite(RED_LED, LOW);
  
  // Timer interrupt setup
  cli(); // stop interrupts

  TCCR1A = 0; // set timer control register A to 0
  TCCR1B = 0; // set timer control register B to 0
  TCNT1 = 0; // initialize timer/counter register to 0

  OCR1A = 3124; // compare register value w/ 1024 prescaler, equivalent to 200ms must be compatible with the value set for TIMER_FSM
  
  TCCR1B |= (1 << WGM12); // set the WGM12 bit of register TCCR1B to enable CTC
  TCCR1B |= (1 << CS12) | (1 << CS10); // set the CS10 and CS12 bit of the register TCCRB to enable prescaler 1024
  TIMSK1 |= (1 << OCIE1A); // set the OCIE1A bit of the TIMSK1 register to enable comparison with register OCR1A

  sei(); //allow interrupts

}

// Functions
ISR(TIMER1_COMPA_vect){

  ++counter;

  if (state == 0 && counter == 5){
    digitalWrite(RED_LED, !digitalRead(RED_LED));
    counter = 0;
  }
  else if (state == 3){
    
    if (counter >= 15)
      digitalWrite(YELLOW_LED, !digitalRead(YELLOW_LED));

    if (counter == 30){
      timerLimit = 20;
      counter = 0;
      state = 1;
      
      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(RED_LED, HIGH);
    }
    
  }
    
  else if (counter >= 100){
    counter = 0;

    switch(state){
      
      case 1:
        digitalWrite(RED_LED, LOW);
        digitalWrite(GREEN_LED, HIGH);
        ++state;
        break;
        
      case 2:
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(YELLOW_LED, HIGH);
        timerLimit = 6;
        ++state;
        break;
      }
  }

  if (state != 0 && counter/FREQ_FSM >= timerLimit - 3) tone(BUZZER, 1000);
  else noTone(BUZZER);
  
} 

void print_digit(unsigned int hex, unsigned int *digit){
    
  digitalWrite(LATCH, LOW);

  //Set digit that will be displayed
  for (int i = 0; i < 4; ++i){

    if (i == *digit) digitalWrite(DIGIT_PINS[i], LOW);
    else digitalWrite(DIGIT_PINS[i], HIGH);
    
    }
  
  shiftOut(DATA, CLCK, LSBFIRST, seven_seg_digits[hex]);

  digitalWrite(LATCH, HIGH);
  
}

void loop() {

  static bool pressed = false;
  static bool prevPress = false;
  static bool pressFlag = true;
  static unsigned int nxtDigit = 0;
  static unsigned int mask = 15;
  static unsigned long currTime = 0;
  static unsigned long prevTime = 0;
  static unsigned long lastPressTime = 0;

  pressed = digitalRead(BUTTON);

  currTime = millis();

  // Button debounce routine
  if (pressed != prevPress){
    lastPressTime = currTime;
    pressFlag = false;
  }
  
  if (currTime - lastPressTime > TIMER_DEBOUNCE){
    if (!pressed && !pressFlag){
      if (state == 0){
        state = 1;
        timerLimit = 20;
        counter = 0;
      }
      else {
        state = 0;
        counter = 0;
        timerLimit = 0;
        digitalWrite(YELLOW_LED, LOW);
        digitalWrite(GREEN_LED, LOW);
        noTone(BUZZER);
      }
      digitalWrite(RED_LED, HIGH);
      pressFlag = true;    
    }
  }

  prevPress = pressed;

  //Display the next hex digit at a given interval
  if (currTime - prevTime >= TIMER_DISPLAY){

    print_digit(((timerLimit - counter/FREQ_FSM) >> nxtDigit*4) & mask, &nxtDigit);
    prevTime = currTime;

    if (nxtDigit == 3) nxtDigit = 0;
    else ++nxtDigit;
    
  }
}
