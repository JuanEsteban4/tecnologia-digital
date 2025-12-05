#include "tft.h"
#include "packetManager.h"

#include "race.h"
#include "zombies.h"
#include "BuzzerManager.h"

#define GAMES 2

int state = 0;          // 0 menu 2 zombies 1 carreras
int menuState = 0;
String op[] = {"Carreras", "Zombies"};
// unsigned long lastMove = 0;   // control de velocidad del menú
// const int MOVE_DELAY = 170;   // milisegundos entre movimientos

void drawMenu() {
  canvas.fillScreen(0);

  // Título
  canvas.setTextSize(3);
  canvas.setTextColor(0x07E0); // verde
  canvas.setCursor(40, 20);
  canvas.println("SELECCIONA");

  canvas.setTextSize(2);
  int y = 80;

  for(int i = 0; i < 2; i++){
    canvas.setCursor(60, y);

    if(i == menuState){
      canvas.setTextColor(0xFFE0); // amarillo resaltado

      // indicador flecha
      canvas.print("> ");
      canvas.println(op[i]);
    } else {
      canvas.setTextColor(0xFFFF);
      canvas.print("  ");
      canvas.println(op[i]);
    }
    y += 30;
  }
}


// Parámetros de suavizado
const int DEADZONE_MIN = 105;
const int DEADZONE_MAX = 145;
bool joyReleased = true;   // para evitar repeticiones rápidas

void handleMenuInput() {
  int y = CONTROL_PACKET.joyY;

  //Button
  bool A = CONTROL_PACKET.buttons & 1;
  if (A) state = menuState + 1;

  // ¿Está dentro de la zona muerta?
  if (y > DEADZONE_MIN && y < DEADZONE_MAX) {
    joyReleased = true;
    return;
  }

  // solo mover si fue "soltado" antes
  if (!joyReleased) return;

  // mover una sola vez
  if (y >= DEADZONE_MAX) {
    menuState = (menuState + 1) % 2;
    joyReleased = false;
  }

  if (y <= DEADZONE_MIN) {
    menuState = (menuState - 1 + 2) % 2;
    joyReleased = false;
  }

}


void menu() {
  handleMenuInput();
  drawMenu();
}

void setup() {
  Serial.begin(115200);
  setupBuzzer();
  initTft();
  setupPacketManager();
}

void loop() {
  if (state == 1){ // Carreras
    initRaceGame();
    loopRaceGame();
    state = 0;
  };
  if (state == 2){
    initZombiesGame();
    loopZombiesGame();
    state = 0;
  };

  menu();
  updateTft();
}