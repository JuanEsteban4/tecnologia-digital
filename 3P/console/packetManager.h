#pragma once

#include <WiFi.h>
#include <esp_now.h>
#include "packet.h"  

controlPacket CONTROL_PACKET;

uint8_t controller[] = {0x10, 0X20, 0xBA, 0x4D, 0x86, 0x30};
unsigned long lastSend = 0;
unsigned long lastReceived = 0;
const unsigned long SEND_INTERVAL_MS = 1000 / 40; // 40 packets per second

void onDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  memcpy(&CONTROL_PACKET, data, sizeof(CONTROL_PACKET));
  lastReceived = millis();
  Serial.printf("b = %d X=%d Y=%d G=%f %f %f\n",CONTROL_PACKET.buttons,CONTROL_PACKET.joyX,CONTROL_PACKET.joyY,CONTROL_PACKET.gyroX, CONTROL_PACKET.gyroY, CONTROL_PACKET.gyroZ);
}

bool addPeer(const uint8_t *mac) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_is_peer_exist(mac)) return true;
  return esp_now_add_peer(&peerInfo) == ESP_OK;
}

void setupPacketManager() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(onDataRecv);

  if (!addPeer(controller)) Serial.println("Controller add failed");
}

void sendPackets() {
  unsigned long now = millis();
  if (now - lastSend >= SEND_INTERVAL_MS) {
    lastSend = now;
    //esp_now_send(controller, (uint8_t*)&CONTROL_PACKET, sizeof(CONTROL_PACKET));

    //IDLE state when no packet
    if (now - lastReceived > 30) {
      CONTROL_PACKET.buttons = 0;
      CONTROL_PACKET.joyX = 128;
      CONTROL_PACKET.joyY = 128;
      CONTROL_PACKET.gyroX = 0;
      CONTROL_PACKET.gyroY = 0;
      CONTROL_PACKET.gyroZ = 0;

    }
  }
}