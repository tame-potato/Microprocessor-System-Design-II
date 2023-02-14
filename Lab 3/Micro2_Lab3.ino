#include <Wire.h>
#include <LiquidCrystal.h>
#include "IRremote.h"

// Constants ///////////////////////////////////////////////////////////////////////////
const int RTC = 0x68 ;
const uint8_t BIT_MASK = 0x0F,
              TIMER_DEBOUNCE = 15, // ms
              IR_DEBOUNCE = 1000, // ms
              RS = 9,
              RW = 8,
              E = 7,
              D0 = 10,
              D1 = 11,
              D2 = 12,
              D3 = 13,
              D4 = 14,
              D5 = 15,
              D6 = 16,
              D7 = 17,
              SIG = 3,
              M1_PWM = 6,
              M1_CW = 4,
              M1_CCW = 5,
              BUTTON = 2;
String currDate;
              

// Global Variables ////////////////////////////////////////////////////////////////////
LiquidCrystal lcd(RS, RW, E, D0, D1, D2, D3, D4, D5, D6, D7);
IRrecv irRemote(SIG);
uint8_t percent = 75;
bool cw = true;

void setup() {

  // L293D SETUP ///////////////////////////////////////////////////////////////////////
  pinMode(M1_PWM, OUTPUT);
  pinMode(M1_CW, OUTPUT);
  pinMode(M1_CCW, OUTPUT);

  // RTC SETUP /////////////////////////////////////////////////////////////////////////
  Wire.begin();

  // LCD SETUP /////////////////////////////////////////////////////////////////////////
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("VEL(%):");
  update_date();
  update_motor_status();

  // IR RECEIVER SETUP /////////////////////////////////////////////////////////////////
  irRemote.enableIRIn();

  // BUTTON INTERRUPT SETUP ////////////////////////////////////////////////////////////
  pinMode(BUTTON, INPUT_PULLUP);
  
  // TIMER INTERRUPT SETUP /////////////////////////////////////////////////////////////
  cli(); // stop interrupts

  TCCR1A = 0; // set timer control register A to 0
  TCCR1B = 0; // set timer control register B to 0
  TCNT1 = 0; // initialize timer/counter register to 0

  OCR1A = 15624; // compare register value w/ 1024 prescaler, equivalent to 1000ms must be compatible with the value set for TIMER_FSM
  
  TCCR1B |= (1 << WGM12); // set the WGM12 bit of register TCCR1B to enable CTC
  TCCR1B |= (1 << CS12) | (1 << CS10); // set the CS10 and CS12 bit of the register TCCRB to enable prescaler 1024
  TIMSK1 |= (1 << OCIE1A); // set the OCIE1A bit of the TIMSK1 register to enable comparison with register OCR1A

  sei(); //allow interrupts

}

// Screen update every 1000ms w/ interrupt /////////////////////////////////////////////
ISR(TIMER1_COMPA_vect){

  update_date();

  update_motor_status();
  
}

// Get the date and time information from the RTC module through I2C////////////////////
String get_RTC(){

  uint8_t secRAW, minRAW, hrRAW, secs, mins, hrs;
  String secsString, minsString, hrsString;

  // Request time registers from RTC through I2C
  Wire.beginTransmission(RTC);
  Wire.write(0x00);
  Wire.endTransmission(false);
  Wire.requestFrom(RTC, 3, true);
  
  secRAW = Wire.read();
  minRAW = Wire.read();
  hrRAW = Wire.read();

  // Decode the received registers
  secs = (secRAW>>4 & BIT_MASK) * 10 + (secRAW & BIT_MASK); 
  mins = (minRAW>>4 & BIT_MASK) * 10 + (minRAW & BIT_MASK); 
  hrs = (hrRAW>>4 & BIT_MASK) * 10 + (hrRAW & BIT_MASK);

  // If single digit quantity add eading zero to mantain constant string length
  if (secs<10) secsString = "0" + String(secs);
  else secsString = String(secs);
  if (mins<10) minsString = "0" + String(mins);
  else minsString = String(mins);
  if (hrs<10) hrsString = "0" + String(hrs);
  else hrsString = String(hrs);

  return hrsString + ":" + minsString + ":" + secsString ;
  
}

// Update time displayed on screen ////////////////////////////////////////////////////
void update_date(){
  
  lcd.setCursor(0, 0);
  lcd.print(currDate);
  
}

// Update motor speed and direction displayed on screen ///////////////////////////////
void update_motor_status(){
  
  lcd.setCursor(7,1);
  lcd.print("   ");
  lcd.setCursor(7,1);
  lcd.print(percent);

  lcd.setCursor(13,1);
  if (cw) lcd.print(" CW");
  else lcd.print("CCW");  
  
}

// Poll pushbutton including a debounce routine //////////////////////////////////////////
bool debounce(unsigned long currTime){

  bool validPress = false;
  static bool pressed = true;
  static bool prevPress = true;
  static bool pressFlag = false;
  static unsigned long prevTime = 0;
  static unsigned long lastPressTime = 0;
  
  pressed = digitalRead(BUTTON);

  // Button debounce routine
  if (pressed != prevPress){
    if (currTime - lastPressTime > TIMER_DEBOUNCE ){
      if (!pressed){
        validPress = true;
      }
    }
    lastPressTime = currTime;
  }
  
  prevPress = pressed;

  return validPress;
  
}

// Check IR receiver for new input and match inputs with actions /////////////////////
void check_IR(){

  decode_results irInput;

  if (irRemote.decode(&irInput)){
    
      // Valid IR inputs
      switch(irInput.value){
        
      // Volume + button
      case 0xFF629D:
        if (percent <= 95) percent += 5;
        break;
    
      // Volume - button
      case 0xFFA857: 
        if (percent >= 10) percent -= 5;    
        break;
    
      // Play/Pause button
      case 0xFF02FD: 
        cw = !cw;    
        break;
    
      }
      
      irRemote.resume();
  }
}


// Main loop /////////////////////////////////////////////////////////////////////////
void loop() {

  static unsigned long currTime;
  static uint8_t level; 

  currTime = millis();

  // Check IR receiver for input
  check_IR();

  // Check for button press and flip direction if detected
  if (debounce(currTime)){
    cw = !cw;  
  }

  // Set direction of rotation based on state of cw variable
  if (cw){
    
    digitalWrite(M1_CW, HIGH);
    digitalWrite(M1_CCW, LOW);
    
  }
  else{

    digitalWrite(M1_CW, LOW);
    digitalWrite(M1_CCW, HIGH);
    
  }

  // 0% represents a PWM output of 90 (30% Duty Cycle) which is the minimum speed at which the motor remains turning
  analogWrite(M1_PWM, percent*165/100 + 90);

  // Update time from RTC
  currDate = get_RTC();

}
