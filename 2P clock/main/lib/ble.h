#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "ledsclock.h"
#include "clock.h"
#include "tail.h"
// Servidor BLE
BLEServer* pServer = NULL;
bool deviceConnected = false;

// UUIDs del servicio y características
#define SERVICE_UUID_LED   "12345678-1234-1234-1234-123456789abc"
#define CHAR_UUID_LED      "abcd1234-4321-abcd-4321-abcdef123456"

#define SERVICE_UUID_TIME  "87654321-4321-4321-4321-cba987654321"
#define CHAR_UUID_TIME     "fedcba98-1234-5678-90ab-cdef12345678"

// Características
BLECharacteristic* pCharLED;
BLECharacteristic* pCharTime;

// ------------------------
//  Callbacks del servidor
// ------------------------
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Dispositivo conectado.");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Dispositivo desconectado.");
    BLEDevice::startAdvertising();
  }
};

// ----------------------------
//  Callback para COLOR (LEDs)
// ----------------------------
class LEDCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String rx = pCharacteristic->getValue();
    if (rx.length() == 0) return;

    String str = String(rx.c_str());
    Serial.print("RGB recibido: ");
    Serial.println(str);

    int r, g, b;
    if (sscanf(str.c_str(), "%d,%d,%d", &r, &g, &b) == 3) {
      setColor(r, g, b);
    } else {
      Serial.println("Error: formato RGB inválido. Debe ser R,G,B");
    }
  }
};

// ----------------------------
//  Callback para la HORA
// ----------------------------
class TimeCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String rx = pCharacteristic->getValue();
    if (rx.length() == 0) return;

    String str = String(rx.c_str());
    Serial.print("Hora recibida: ");
    Serial.println(str);

    int sec, min, hour , day, month, year;
    int mode;
    if (sscanf(str.c_str(), "%d/%d/%d %d:%d:%d", &day, &month, &year, &hour, &min, &sec) == 6) {
      //Call clock funct
      setClockTime(hour,min,sec);
      setClockDate(day,month,year);

    } else if (sscanf(str.c_str(), "%d", &mode) == 1){
      if (mode < 3){
        setClockMode(mode);
      }else{
        toggleActive();
      }
    }else {
      Serial.println("Error: formato Date inválido. Debe ser dd/mm/yyyy hh:mm:ss or Mode (0,1,2)");
    }
  }
};

// ------------------------
//       SETUP
// ------------------------
void BLE_setup() {
  //Serial.begin(115200);
  initLeds();

  BLEDevice::init("Cheshire Clock");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // --------- Servicio LED ---------
  BLEService *pServiceLED = pServer->createService(SERVICE_UUID_LED);

  pCharLED = pServiceLED->createCharacteristic(
               CHAR_UUID_LED,
               BLECharacteristic::PROPERTY_WRITE
             );
  pCharLED->setCallbacks(new LEDCallbacks());
  pServiceLED->start();

  // --------- Servicio TIME ---------
  BLEService *pServiceTime = pServer->createService(SERVICE_UUID_TIME);

  pCharTime = pServiceTime->createCharacteristic(
                CHAR_UUID_TIME,
                BLECharacteristic::PROPERTY_WRITE
              );
  pCharTime->setCallbacks(new TimeCallbacks());
  pServiceTime->start();

  // --------- Advertising ---------
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID_LED);
  pAdvertising->addServiceUUID(SERVICE_UUID_TIME);

  BLEDevice::startAdvertising();

  Serial.println("BLE listo con dos características.");
}

// ------------------------
//         LOOP
// ------------------------
/*void loop() {
  unsigned long now = millis();
  stepCircleAnimation(now);
  tickPerSecond(now);
}*/
