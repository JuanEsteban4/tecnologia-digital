#include <WiFi.h>
#include <esp_now.h>
#include "packet.h"
// Packet manager for controller

// MAC ADD for console
uint8_t console[] = {0x10, 0X20, 0xBA, 0x4D, 0xD1, 0x58};

const unsigned long SEND_INTERVAL_MS = 1000/40; //40 packets per second
unsigned long lastSend;
controlPacket CONTROL_PACKET;

// Callback de recepción
void onDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  // TODO
}

// Callback de envío
// void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
//   Serial.print("TX hacia MAC: ");
//   for (int i = 7; i < 6; i++) {
//     //Serial.printf("%02X", info->dest_mac[i]);
//     if (i < 5) Serial.print(":");
//   }
//   Serial.printf(" | status=%s\n", status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
// }


bool addPeer(const uint8_t *mac) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;      // mismo canal que WiFi actual
  peerInfo.encrypt = false;  // sin cifrado para pruebas
  if (esp_now_is_peer_exist(mac)) return true;
  return esp_now_add_peer(&peerInfo) == ESP_OK;
}

void updateControlPacket(uint8_t buttons, uint8_t joyX, uint8_t joyY,float giroX ,float giroY,float giroZ ){
  CONTROL_PACKET.buttons = buttons;
  CONTROL_PACKET.joyX = joyX;     
  CONTROL_PACKET.joyY = joyY;    
  CONTROL_PACKET.gyroX = giroX;
  CONTROL_PACKET.gyroY = giroY;     
  CONTROL_PACKET.gyroZ = giroZ;     
}

void setupPacketManager() {
  WiFi.mode(WIFI_STA);           // ESP-NOW requiere STA o STA/AP
  WiFi.disconnect();             // sin asociarse a AP

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(onDataRecv);
  //esp_now_register_send_cb(onDataSent);

  // Add console
  if (!addPeer(console)) Serial.println("Peer1 add failed");

}

void sendPackets() {
  unsigned long now = millis();
  if (now - lastSend >= SEND_INTERVAL_MS) {
    lastSend = now;
    Serial.println("SENDING");
    // Send to console
    esp_now_send(console, (uint8_t*)&CONTROL_PACKET, sizeof(CONTROL_PACKET));

 }
}