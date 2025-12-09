// Pines del joystick
#define VRX_PIN 10   // eje X
#define VRY_PIN 9  // eje Y

void setupJoystick(){
    pinMode(VRX_PIN, INPUT);
    pinMode(VRY_PIN, INPUT);
}

// Swapped due to control built
int getJoyY(){
  return analogRead(VRX_PIN);  // 0 - 4095
}

int getJoyX(){
  return analogRead(VRY_PIN);  // 0 - 4095
}
