#ifndef SLOT_MACHINE_H
#define SLOT_MACHINE_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "sprites.h"
#include "util.h"

#define SPIN_SPEED 60

GFXcanvas16 canvas(240, 240);

// ===========================================================
// ==================== Clase Carrousel ======================
// ===========================================================
class Carrousel {
private:
  int mid = 0;
  int offset = 0;
  int size = NUM_SPRITES;
  int column;

  // Movimiento y animación
  bool spinning = false;
  unsigned long startTime = 0;
  unsigned long spinDuration = 0;
  float speed = 0;
  float targetSpeed = 0;

  // Helper para index circular seguro
  int circularIndex (int index) const {
    index %= size;
    if (index < 0) index += size; // corrige negativos
    return index;
  };

public:
  Carrousel(int col) {
    column = col;
    mid = random(0, size + 1);
  }

  void draw() {
    int i_top2 = circularIndex(mid - 2);
    int i_top1 = circularIndex(mid - 1);
    int i_mid  = circularIndex(mid);
    int i_bot1 = circularIndex(mid + 1);
    int i_bot2 = circularIndex(mid + 2);

    canvas.drawRGBBitmap(0 + (column * 80),   -80 + offset,  all_sprites[i_top2], 80, 80);
    canvas.drawRGBBitmap(0 + (column * 80),    0 + offset,   all_sprites[i_top1], 80, 80);
    canvas.drawRGBBitmap(0 + (column * 80),   80 + offset,   all_sprites[i_mid],  80, 80);
    canvas.drawRGBBitmap(0 + (column * 80),  160 + offset,   all_sprites[i_bot1], 80, 80);
  }

  void move(int pixels) {
    offset += pixels;
    if (offset > 80) {
      next();
      offset -= 80;
    }
  }

  void next() {
    mid = (mid - 1) % size;
  }

  void startSpin(unsigned long time) {
    if (spinning) return;
    spinning = true;
    startTime = millis();
    spinDuration = time;
    speed = SPIN_SPEED;
    targetSpeed = 0;
  }

  void update() {
    if (!spinning) return;

    unsigned long now = millis();
    unsigned long elapsed = now - startTime;

    // Desaceleración progresiva
    if (elapsed < spinDuration) {
      if (speed > 2) speed -= 0.05;
    } else {
      // tiempo cumplido → desacelerar hasta parar
      if (speed > 5) speed -= 1;
      else {
        int remainder = offset % 80;
        if (remainder > 5) move(5);
        else spinning = false;
      }
    }

    move((int)speed);
    draw();
  }

  bool isSpinning() const { return spinning; }
  int getMid() const { return circularIndex(mid); }
};

// ===========================================================
// ================== Clase SlotMachine ======================
// ===========================================================
class SlotMachine {
private:
  Carrousel c1;
  Carrousel c2;
  Carrousel c3;

public:
  SlotMachine() : c1(0), c2(1), c3(2) {}

  void draw() {
    c1.draw();
    c2.draw();
    c3.draw();
    drawTransparentBitmap(canvas,0,0,bg,240,240);
  }

  void update() {
    c1.update();
    c2.update();
    c3.update();
    draw();
  }

  void play() {
    unsigned long t = random(5000, 8000);
    unsigned long t1 = random(2000, 3000);
    c1.startSpin(t);
    c2.startSpin(t + t1);
    c3.startSpin(t + t1 * 2);
  }

  bool isSpinning() const {
    return c1.isSpinning() || c2.isSpinning() || c3.isSpinning();
  }

  bool hasWon() const {
    int a = c1.getMid();
    int b = c2.getMid();
    int c = c3.getMid();
    return (a == b && b == c);
  }
};

#endif