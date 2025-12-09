#pragma once

#include <ESP32Servo.h>
#include "BuzzerManager.h"
Servo servo;
int pin_servo = 16;

void initServo(){
  servo.attach(pin_servo);
  //servo.write(90); 
}

void rotateServo(int degrees){ //0 -180
  //ledcWriteTone(BUZZER_PIN, 0); //interferencia 
  servo.write(degrees) ;
}
