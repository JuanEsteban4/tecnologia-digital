#include <Wire.h>
#include <MPU6050_light.h>

#define SDA 4
#define SCL 5

unsigned long lastGyroUpdate = 0;
const unsigned long GYRO_INTERVAL = 1; // cada 15 ms

MPU6050 gyroscope(Wire);

void setupGyro() {
  Wire.begin(SDA, SCL);

  byte status = gyroscope.begin();
  Serial.print("MPU Status: ");
  Serial.println(status);
  while (status != 0) delay(500);

  Serial.println("Calibrando...");
  gyroscope.calcOffsets(true, true);
  Serial.println("Calibrado!");
}

void updateGyro() {
    unsigned long now = millis();
    if (now - lastGyroUpdate >= GYRO_INTERVAL) {
        lastGyroUpdate = now;
        gyroscope.update();
    }
}
float getGyroX() {
  return gyroscope.getAngleX();
}
float getGyroY() {
  return gyroscope.getAngleY();
}
float getGyroZ() {
  return gyroscope.getAngleZ();
}
