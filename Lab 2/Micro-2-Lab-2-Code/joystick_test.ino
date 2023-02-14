
int VRx = A0;
int VRy = A1;
int buzzer= 3;

int X = 0;
int Y = 0;
//int SW_state = 0;

void setup() {
  Serial.begin(9600);
  pinMode(VRx, INPUT);
  pinMode(VRy, INPUT);
  

}

void loop() {
  X = analogRead(VRx);
  Y = analogRead(VRy);

 if (Serial.read()== 'E')
  {
    
    tone(buzzer, 2000, 1000); 
    
    }
  
  
  if(Y > 800)
  {  
    Serial.print("S"); // If joystick is pushed up, go up
    delay(100);
  }
  
  
  else if(Y < 200)
  {
    Serial.print("W");// If joystick is pushed down, go down
    delay(100);
  }
  
  else if(X <200)
  {
    Serial.print("A"); // If joystick is pushed left, go left
    delay(100); 
  }
  
  else if(X > 800)
  {
    Serial.print("D"); // If joystick is pushed right, go right
    delay(100);
  }

}
