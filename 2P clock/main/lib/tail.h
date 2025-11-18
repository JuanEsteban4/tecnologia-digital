#include <ESP32Servo.h>

Servo servo;
int pin_servo = 8;
int degrees[] = {0,60};
int d_state = 0;
bool active = false;

void initServo(){
  servo.attach(pin_servo); 
}

void tickServo(){
  if(!active){
    return;
  }
  d_state = (d_state + 1) % 2;
  servo.write(degrees[d_state]);  
}

void toggleActive(){
    active = !active;
    Serial.println(active);
}
