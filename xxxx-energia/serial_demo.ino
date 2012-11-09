#include "Servo.h"

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  Serial.print("test aaa\r\n");
  delay(2000);
  Serial.print("test bbb\r\n");
  delay(2000);
}


