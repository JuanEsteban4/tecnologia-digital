#include <Adafruit_NeoPixel.h>

#define LED_PIN     9
#define NUM_LEDS    16
#define BRIGHTNESS  80

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Variables para timing no bloqueante
unsigned long previousMillis = 0;
const long rainbowInterval = 30;  // ms entre actualizaciones
uint16_t rainbowHue = 0;          // Posición actual en el arcoíris

void setupNeopixel() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show(); 
}

// Función HSV a RGB optimizada
uint32_t hsvToRgb(uint16_t h, uint8_t s = 255, uint8_t v = 255) {
  h = (h * 1536L) / 256;  // Convertir 0-255 a 0-1536
  
  uint8_t r, g, b;
  
  if (h < 256) {          // Rojo → Amarillo
    r = 255;
    g = h;
    b = 0;
  } else if (h < 512) {   // Amarillo → Verde
    r = 511 - h;
    g = 255;
    b = 0;
  } else if (h < 768) {   // Verde → Cian
    r = 0;
    g = 255;
    b = h - 512;
  } else if (h < 1024) {  // Cian → Azul
    r = 0;
    g = 1023 - h;
    b = 255;
  } else if (h < 1280) {  // Azul → Magenta
    r = h - 1024;
    g = 0;
    b = 255;
  } else {                // Magenta → Rojo
    r = 255;
    g = 0;
    b = 1535 - h;
  }
  
  // Ajustar saturación y valor (simplificado)
  if (s < 255) {
    r = (r * s + 255 * (255 - s)) / 255;
    g = (g * s + 255 * (255 - s)) / 255;
    b = (b * s + 255 * (255 - s)) / 255;
  }
  
  if (v < 255) {
    r = (r * v) / 255;
    g = (g * v) / 255;
    b = (b * v) / 255;
  }
  
  return strip.Color(r, g, b);
}

// Actualización no bloqueante del rainbow
void updateRainbow() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= rainbowInterval) {
    previousMillis = currentMillis;
    
    // Asignar color rainbow a cada LED con desplazamiento
    for(int i = 0; i < NUM_LEDS; i++) {
      // Cada LED tiene un desplazamiento diferente
      uint16_t ledHue = (rainbowHue + (i * 256 / NUM_LEDS)) & 255;
      strip.setPixelColor(i, hsvToRgb(ledHue));
    }
    
    // Mostrar cambios
    strip.show();
    
    // Incrementar posición en el rainbow
    rainbowHue = (rainbowHue + 1) & 255;  // Wrap a 255
  }
}

void loopNeopixel() {
  updateRainbow();
}