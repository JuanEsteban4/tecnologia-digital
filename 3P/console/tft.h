#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Pines TFT
#define TFT_DC    6
#define TFT_RST   5
#define TFT_CS   10

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(240, 240);

void initTft() {
  tft.init(240, 240, SPI_MODE3);
  tft.setRotation(2);
  tft.fillScreen(0);
  canvas.setTextSize(3);
}

void updateTft() {
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 240);
}
