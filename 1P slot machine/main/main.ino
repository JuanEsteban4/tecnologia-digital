#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Bounce2.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#include "slot_machine.h"

// Pines ESP32-S3
#define TFT_DC    6
#define TFT_RST   5
#define TFT_CS   10

//Leds
#define PIN        9
#define MAX_BRIGHTNESS 50
#define NUMPIXELS 32
#define DELAYVAL 100
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint32_t idleColor1 = pixels.Color(0, 30, 80);   // Original blue
uint32_t idleColor2 = pixels.Color(10, 150, 60); // Original green


Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define BUTTON_PIN 38
#define BUTTON_PIN2 39

// Creamos un objeto tipo Button
Bounce2::Button button = Bounce2::Button();
Bounce2::Button button2 = Bounce2::Button();

SlotMachine game;

// Estado de LEDs
enum LedState { LED_IDLE, LED_SPIN, LED_RESULT_WIN, LED_RESULT_LOSE };
LedState ledState = LED_IDLE;
unsigned long resultStart = 0;
const unsigned long RESULT_DISPLAY_MS = 3000; // muestra resultado 3s
bool wasSpinning = false;

// Animación LEDs (no bloqueante)
unsigned long lastLedUpdate = 0;
const unsigned long LED_ANIM_MS = 80;
int animIndex = 0;

void setup() {
  Serial.begin(115200);

  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
  #endif
  pixels.begin(); 

  tft.init(240, 240, SPI_MODE3);  
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);
  game.draw();
  // Configurar el botón
  button.attach(BUTTON_PIN, INPUT_PULLUP);  // usa pull-up interno
  button.interval(25);                      // tiempo de anti-rebote
  button.setPressedState(LOW);              // porque está conectado a GND

  button2.attach(BUTTON_PIN2, INPUT_PULLUP);  // usa pull-up interno
  button2.interval(25);                      // tiempo de anti-rebote
  button2.setPressedState(LOW);              // porque está conectado a GND


  pixels.setBrightness(MAX_BRIGHTNESS);
  for (int i = 0; i < NUMPIXELS; i++) {
    // pares = azul tenue, impares = verde "casino"
    if (i % 2 == 0) pixels.setPixelColor(i, pixels.Color(0, 30, 80));
    else pixels.setPixelColor(i, pixels.Color(10, 150, 60));
  }
  pixels.show();
}

void loop() {
  // Actualizar botones y juego
  button.update();
  button2.update();


  // Manejar pulsación: iniciar juego y cambiar estado de LEDs
  if ((button.pressed()) && !game.isSpinning()) {
    game.play();
    ledState = LED_SPIN;
  }

  if (button2.pressed()) {
    if ( !game.isSpinning()) {
    // Generate new random colors for idle mode
    idleColor1 = getRandomColor();
    idleColor2 = getRandomColor();
  }

}
  game.update();

  // Detectar transición: estaba girando y ya no -> evaluar resultado
  if (game.isSpinning()) {
    wasSpinning = true;
  } else {
    if (wasSpinning) {
      // acaba de parar
      if (game.hasWon()) {
        ledState = LED_RESULT_WIN;
      } else {
        ledState = LED_RESULT_LOSE;
      }
      resultStart = millis();
      wasSpinning = false;
    }
  }

  // Volver a estado inicial si se pierde o se gana (led_idle)
  if ((ledState == LED_RESULT_WIN || ledState == LED_RESULT_LOSE) &&
      (millis() - resultStart > RESULT_DISPLAY_MS)) {
    ledState = LED_IDLE;
  }

  
  leds();

  // Actualizar pantalla
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 240);
}

void leds(){
  unsigned long now = millis();

  // Animación simple: actualiza cada LED_ANIM_MS ms
  if (now - lastLedUpdate < LED_ANIM_MS) return;
  lastLedUpdate = now;

  switch (ledState) {
    case LED_IDLE:
      // Estado inicial
      for (int i = 0; i < NUMPIXELS; i++) {
    // Led pares color1, impares color2
      if (i % 2 == 0) pixels.setPixelColor(i, idleColor1);
      else pixels.setPixelColor(i, idleColor2);
    }
    break;

    case LED_SPIN: {
      
      // Patron por secciones: 0-7, 8-15, 16-31    
      animIndex = (animIndex + 1) % 8; // 8 pasos en el patrón
      // colores RGB
      const uint8_t YR = 255; 
      const uint8_t YG = 180; 
      const uint8_t YB = 20;  

      //Inicio de cada seccion
      const int starts[] = {0, 8, 16};
      // Longitud de cada seccion(tira-tira-matriz)
      const int lengths[] = {8, 8, 16};

      for (int i = 0; i < NUMPIXELS; i++) {
        // determinar a qué sección pertenece i
        int colorSet = 0;
        int rel = 0;
        for (int s = 0; s < 3; s++) {
          int st = starts[s];
          int len = lengths[s];
          if (i >= st && i < st + len) {
            // Hacer modulo para dar la vuelta
            rel = (i - st) % 8; 
            colorSet = 1;
            break;
          }
        }

        if (!colorSet) {
          // fuera de las secciones (no debería ocurrir con NUMPIXELS=32)
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
          continue;
        }

        // brillo dependiente de la distancia al punto activo (animIndex)
        int diff = (rel - animIndex + 8) % 8;
        if (diff == 0) {
          // pixel principal: amarillo casino brillante
          pixels.setPixelColor(i, pixels.Color(YR, YG, YB));
        } else if (diff == 1 || diff == 7) {
          // borde próximo: menos brillante
          pixels.setPixelColor(i, pixels.Color(YR/2, YG/2, YB/2));
        } else {
          // tenue
          pixels.setPixelColor(i, pixels.Color(3, 8, 0));
        }
      }
    }
    break;

    case LED_RESULT_LOSE:
      for (int i = 0; i < NUMPIXELS; i++) pixels.setPixelColor(i, pixels.Color(200, 0, 0));
      break;

    case LED_RESULT_WIN:
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 255, 120));
      }
      break;
  }

  pixels.show();
}

uint32_t getRandomColor() {
  const uint32_t dimColors[] = {
    pixels.Color(30, 10, 50),   // Purple
    pixels.Color(10, 30, 60),   // Blue
    pixels.Color(10, 50, 30),   // Green
    pixels.Color(50, 30, 10),   // Orange
    pixels.Color(40, 10, 40),   // Magenta
    pixels.Color(10, 40, 40),   // Teal
    pixels.Color(40, 40, 10),   // Gold
    pixels.Color(20, 20, 40)    // Lavender
  };
  return dimColors[random(0, 8)];
}