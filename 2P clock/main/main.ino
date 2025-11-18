#include "lib/ble.h"
#include "lib/buzzer.h"

void setup(){
  Serial.begin(115200);
  initServo();
  buzzerSetup();
  initLeds();
  BLE_setup();
  setupClock("NTP","12345678");
}

int pastMin = -1;
int pastSec = -1;
void loop(){
  unsigned long now = millis();
  
  int sec = currentTimeTM.tm_sec;
  int min = currentTimeTM.tm_min;
  int hour = currentTimeTM.tm_hour;

  // For buzzer
  if(min == 30 && pastMin != min) { 
    setBuzzerTimes(1);
    pastMin = min;
  }

  if(!min && pastMin != min){
    setBuzzerTimes(hour);
    pastMin = min;
  }

  //For tail movement and flash led
  if(sec != pastSec){
    pastSec = sec;
    tickServo();
    tickPerSecond();
  }

  updateClock();

  //Check if we need to buzzer
  handleBuzzer(now);

  //Updates circle color according to second
  updateProgressLeds(sec);

  //Turns off the second flash after 300ms
  updateFlash(now);

  //Sends leds info in ram to the leds
  leds.show();
}