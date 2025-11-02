#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Bounce2.h>

#include "slot_machine.h"

// Pines ESP32-S3
#define TFT_DC    6
#define TFT_RST   5
#define TFT_CS   10

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define BUTTON_PIN 38
// Creamos un objeto tipo Button
Bounce2::Button button = Bounce2::Button();


SlotMachine game;

void setup() {
  Serial.begin(115200);

  tft.init(240, 240, SPI_MODE3);  
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);
  game.draw();
  // Configurar el botón
  button.attach(BUTTON_PIN, INPUT_PULLUP);  // usa pull-up interno
  button.interval(25);                      // tiempo de anti-rebote
  button.setPressedState(LOW);              // porque está conectado a GND
}

void loop() {
  button.update(); 
  game.update();

  if(button.pressed() && !game.isSpinning()){
    game.play();
  }

  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 240);
}