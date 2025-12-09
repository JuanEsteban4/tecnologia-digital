#pragma once

#include <WiFi.h>
#include <ThingSpeak.h>

#define CHANNEL 3196711
#define API_KEY "EMTI3GZSAUPXYVMT"

const char* ssid = "console";
const char* password = "qwertyuiop";

WiFiClient client;
unsigned long lastSentTS;

void setupTS() {

  // --- WiFi ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println(" Connected.");

  ThingSpeak.begin(client);
}

void TSsendUpdate(int gx, int gy, int gz){
  // Send to ThingSpeak
  unsigned long now = millis();
  if (now - lastSentTS < 15000) return;

  ThingSpeak.setField(1, gx);
  ThingSpeak.setField(2, gy);
  ThingSpeak.setField(3, gz);

  int code = ThingSpeak.writeFields(CHANNEL, API_KEY);
  lastSentTS = now;
  if (code == 200)
    Serial.println("ThingspeakUpdate successful");
  else
    Serial.printf("Error: %d\n", code);
}