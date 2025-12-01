// Pines del joystick
#define VRX_PIN 10   // eje X
#define VRY_PIN 9  // eje Y

void setupJoystick(){
    pinMode(VRX_PIN, INPUT);
    pinMode(VRY_PIN, INPUT);
}
int getJoyX(){
  return analogRead(VRX_PIN);  // 0 - 4095
}

int getJoyY(){
  return analogRead(VRY_PIN);  // 0 - 4095
}
