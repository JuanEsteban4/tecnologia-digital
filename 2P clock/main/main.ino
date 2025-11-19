#include "lib/ble.h"
#include "lib/buzzer.h"

void setup(){
  //Serial.begin(115200);
  initServo();
  buzzerSetup();
  initLeds();
  BLE_setup();
  setupClock("NTP","12345678");
}

int pastMin = -1;
int pastSec = -1;
unsigned long lastmillis = 0;
void loop(){
  unsigned long now = millis();
  
  int sec = currentTimeTM.tm_sec;
  int min = currentTimeTM.tm_min;
  int hour = currentTimeTM.tm_hour;

  //Auto change mode every 15 sec
  if((now - lastmillis) > 15000){
    setClockMode((clockDisplayMode+1) % 3);
    lastmillis = now;
  }
  // For buzzer
  if(min == 30 && pastMin != min) { 
    setBuzzerTimes(1);
    pastMin = min;
  }

  if(!min && pastMin != min){
    int times = (hour % 12 == 0) ? 12 : hour % 12;
    setBuzzerTimes(times);
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