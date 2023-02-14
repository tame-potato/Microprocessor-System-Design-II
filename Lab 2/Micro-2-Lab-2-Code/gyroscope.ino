#include <Wire.h> 

const int MPU=0x68,  // I2C address of the MPU-6050
          buzzer = 3; // Buzzer digital pin

float acX, 
      acY, 
      acZ, 
      acRoll,
      acPitch,
      maxG = 0;

void initIMU(){

  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);  // Select 0x6B PWR_MGMT_1 register
  Wire.write(0x00);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  
}

void readIMU(){

  static const float acDiv = 16384; // Divide accelerometer readings by this value when in default range +/-2g
                   
  // Request data
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU,6,true);  // request 6 registers

  // Read raw data and scale it
  acX = (Wire.read()<<8|Wire.read()) / acDiv;  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
  acY = (Wire.read()<<8|Wire.read()) / acDiv;  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  acZ = (Wire.read()<<8|Wire.read()) / acDiv;  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)

  // Convert acc readings to roll/pitch
  acRoll = (atan(acY / sqrt(pow(acX, 2) + pow(acZ, 2))) * 180 / PI); 
  acPitch = (atan(-1 * acX / sqrt(pow(acY, 2) + pow(acZ, 2))) * 180 / PI);
  
}

void setup()
{

  Serial.begin(9600);

  initIMU();

}

void loop()
{

  static const int angleThresh = 30;
  static const float shakeThresh = 1.5;

  char prev = 'Z';

  if (Serial.read()== 'E') tone(buzzer, 2000, 1000); 
  
  readIMU();

  if (acRoll > angleThresh && prev != 'A'){
    Serial.print("A");
    prev = 'A';
  }
  else if (acRoll < -angleThresh && prev != 'D'){
    Serial.print("D");
    prev = 'D';
  }

  if (acPitch > angleThresh && prev != 'S'){
    Serial.print("S");
    prev = 'S';
  }
  else if (acPitch < -angleThresh && prev != 'W'){
    Serial.print("W");
    prev = 'W';
  }

  if (sqrt(pow(acX,2) + pow(acY,2) + pow(acZ,2)) > shakeThresh) Serial.print("G");

  delay(100);
  
}
