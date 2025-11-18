#include <Adafruit_NeoPixel.h>
#include <map>
// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        9 

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 17 // Popular NeoPixel ring size + led for tick per second

int currentLed = 0;
unsigned long lastUpdate = 0;

int maxBright = 25;

// Paleta NeoPixel
// Formato: { nombre_color : {r, g, b} }
struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

Color circleColor = { 255,  30, 140 };

Adafruit_NeoPixel leds(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void initLeds() {
  leds.begin();
  leds.setBrightness(maxBright);
}

void updateProgressLeds(int sec){
  currentLed = map(sec, 0, 59, 0, 15); // 0 LED → 59 LEDs

  for(int i = 0; i < 16; i++){
    if(i < currentLed){
      leds.setPixelColor(i, leds.Color(circleColor.r, circleColor.g, circleColor.b));
    } else {
      leds.setPixelColor(i, 0); // apagado
    }
  }

  // Mantener funcionalidad de flash
  /*if(flashActive){
    leds.setPixelColor(16, leds.Color(circleColor.r, circleColor.g, circleColor.b));
  }*/

  leds.show();
}

bool flashActive = false;
unsigned long flashStart = 0;
unsigned long flashDuration = 200; // ms

void tickPerSecond() {
  // arranca el pulso en el LED 16
  flashActive = true;
  flashStart = millis();
  leds.setPixelColor(16, leds.Color(circleColor.r, circleColor.g, circleColor.b));
}

void updateFlash(unsigned long now) {
  if (flashActive && (now - flashStart) > flashDuration) {
    flashActive = false;
    leds.setPixelColor(16, 0);
  }
}

void setColor(int r,int g, int b){
  circleColor = {r, g, b};

  for(int i = 0; i < currentLed; i++){
    leds.setPixelColor(i, leds.Color(circleColor.r, circleColor.g, circleColor.b));
  }
  if(flashActive){
    leds.setPixelColor(16, leds.Color(circleColor.r, circleColor.g, circleColor.b));
  }
}