#include "packetManager.h"
#include "joystick.h"
#include "gyro.h"

#define bA 6
#define bB 7
#define bBack 15 
#define bPause 16

void setup(){
  Serial.begin(115200);
  setupPacketManager(); 
  setupJoystick();
  setupGyro();

  //Buttons
  pinMode(bA,INPUT_PULLUP);
  pinMode(bB,INPUT_PULLUP);
  pinMode(bBack,INPUT_PULLUP);
  pinMode(bPause,INPUT_PULLUP);
}

void loop(){
  // 1. get info
  uint8_t buttons, joyX, joyY ;
  uint16_t giro;

  bool A, B, b, p;
  A = (digitalRead(bA) == LOW);
  B = (digitalRead(bB) == LOW);
  b = (digitalRead(bBack) == LOW);
  p = (digitalRead(bPause) == LOW);

  buttons = A | (B << 1) | (b << 2) | (p << 3);

  joyX = map(getJoyX(), 0, 4095, 0, 255);
  joyY = map(getJoyY(), 0, 4095, 0, 255);

  updateGyro();
  giro = getGyroX();

  // 2. send info
  updateControlPacket(buttons,joyX,joyY,giro);
  sendPackets();
}