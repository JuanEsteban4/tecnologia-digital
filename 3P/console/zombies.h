/*
  Raycast ESP32-S3 + ST7789 (240x240) using Adafruit_GFX + GFXcanvas16
  Controls:
   - Joystick analog X/Y (0..255 expected). Mapped to rotation (X) and forward/back (Y).
   - BTN_PAUSE, BTN_BACK, BTN_A, BTN_B (digital buttons, pull-down assumed)
  Adjust pins if needed.
*/
#pragma once

#include "tft.h"
#include "sprites.h"  
#include "packetManager.h"
#include "buzzerManager.h"


#include "zombies_buzzer.h"

// ---------- Config y constantes ----------
#define PI 3.14159265359f
#define P2 1.57079632679f
#define P3 4.71238898038f

#define WIDTH 240
#define HEIGHT 240

#define VELOCITY 3.0f
#define ANGLE_VELOCITY 0.0872665f // 5 grados
#define MAP_SIZE 15
#define BLOCK 16
#define SHIFT 4
#define FOV 1.0472f // 60 grados
#define RAYS 240

// ---------- Input pins (ajusta a tu placa) ----------
// #define JOY_X_PIN 34   // ADC pin (ejemplo)
// #define JOY_Y_PIN 35   // ADC pin (ejemplo)

// #define BTN_PAUSE 14
// #define BTN_BACK  12
// #define BTN_A     13
// #define BTN_B     15

// ---------- Datos de juego ----------
struct Player { float x,y,a; float dx,dy; } p;
struct Entity { float x,y,z; } entities[1];

float depth[RAYS];

int walls[] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

// ---------- Utilidades ----------
inline float distf(float x1,float y1,float x2,float y2){
  float dx = x1-x2; float dy = y1-y2;
  return sqrtf(dx*dx + dy*dy);
}

// Convertir color RGB565 directo (uint16_t) a formato canvas (ya es 16-bit compatible).
// canvas.drawPixel acepta 16-bit color; usamos el valor tal cual.
inline uint16_t rgb565_from_texture(uint16_t c){
  return c;
}

// Limpiar canvas
void clearcanvas(uint16_t color = 0x0000){
  canvas.fillScreen(color);
}

// Dibuja un pixel seguro en canvas
inline void plot(int x, int y, uint16_t color) {
  if(x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) canvas.drawPixel(x, y, color);
}

void initGame(){
  p.x = 120; p.y = 120; p.a = -PI/2;
  p.dx = cosf(p.a) * VELOCITY;
  p.dy = sinf(p.a) * VELOCITY;
  entities[0].x = 120; entities[0].y = 32; entities[0].z = 120;
}

// ---------- Rendering: minimap y jugador (overlay) ----------
// void drawMap2D(){
//   int mapPixelSize = 6; // tamaño visual del minimapa (ajustable)
//   int mapOffsetX = WIDTH - (MAP_SIZE * mapPixelSize) - 4; // esquina derecha superior
//   int mapOffsetY = 4;
//   for(int mx=0; mx<MAP_SIZE; mx++){
//     for(int my=0; my<MAP_SIZE; my++){
//       int idx = my*MAP_SIZE + mx;
//       uint16_t c = (walls[idx] > 0) ? 0xFFFF : 0x0000; // blanco/negro
//       int xo = mapOffsetX + mx * mapPixelSize;
//       int yo = mapOffsetY + my * mapPixelSize;
//       canvas.fillRect(xo, yo, mapPixelSize-1, mapPixelSize-1, c);
//     }
//   }
//   // Player on minimap
//   int px = mapOffsetX + (int)(p.x / BLOCK * mapPixelSize);
//   int py = mapOffsetY + (int)(p.y / BLOCK * mapPixelSize);
//   canvas.fillCircle(px, py, 3, 0xfc8f);
// }

// // Dibujar jugador en minimapa con dirección
// void drawPlayerMinimap(){
//   int mapOffsetX = WIDTH - (MAP_SIZE * 6) - 4;
//   int mapOffsetY = 4;
//   int px = mapOffsetX + (int)(p.x / BLOCK * 6);
//   int py = mapOffsetY + (int)(p.y / BLOCK * 6);
//   int dx = (int)(cosf(p.a)*8);
//   int dy = (int)(sinf(p.a)*8);
//   canvas.drawLine(px, py, px+dx, py+dy, 0xfc8f);
//   canvas.fillCircle(px, py, 2, 0xfc8f);
// }

// ---------- Draw Entities (sprite) simplified ----------
void drawEntities(){
  float dx = entities[0].x - p.x;
  float dy = entities[0].y - p.y;
  float dz = entities[0].z - (HEIGHT/2);

  float Cos = cosf(p.a), Sin = sinf(p.a);
  float rx = dx * Cos + dy * Sin;
  float ry = -dx * Sin + dy * Cos;

  if(rx <= 0.5f) return; // behind camera

  // proyección simple
  float f = (WIDTH/2) / tanf(FOV/2.0f);
  int screenX = (int)((ry * f / rx) + (WIDTH/2));
  int screenY = (int)((dz * f / rx) + (HEIGHT/2));

  // escala
  float spriteConstant = 3000.0f;
  int scale = (int)(spriteConstant / rx);
  if(scale < 4) scale = 4;

  int texture_index = 2; // assume sprite at index 2 (como en tu código)
  int offset = texture_index * 32 * 32;

  float t_x = 0.0f;
  float t_x_step = 32.0f / (float)scale;
  float t_y_step = 32.0f / (float)scale;
  for(int sx = screenX - scale/2; sx < screenX + scale/2; sx++){
    float t_y = 0.0f;
    for(int sy = screenY - scale/2; sy < screenY + scale/2; sy++){
      if(sx >= 0 && sx < WIDTH && sy >= 0 && sy < HEIGHT && rx < depth[sx]){
        int ti = ((int)t_y) * 32 + (int)t_x + offset;
        if(ti >= 0 && ti < (32*32*16)) { // protección si sprites.h tiene distinto tamaño; ajustar si es necesario
          uint16_t pixel = zombies[ti];
          // magenta 0xF81F es transparente en tu sistema
          if(pixel != 0xF81F){
            plot(sx, sy, rgb565_from_texture(pixel));
          }
        }
      }
      t_y += t_y_step;
    }
    t_x += t_x_step;
  }
}

// ---------- Raycaster principal ----------
void drawRays(){
  // posición en matriz
  int pxMatrix = ((int)p.x) >> SHIFT;
  int pyMatrix = ((int)p.y) >> SHIFT;

  float angle = p.a - FOV/2.0f;

  for(int r=0; r < RAYS; r++){
    // vertical check
    float Tan = tanf(angle);
    float vx, vy, vxo, vyo, dv;
    dv = 1e6f;
    int checks = 0;
    int mx,my,mapWallsIndex;
    int textureIndex = 0;

    if(cosf(angle) > 0.001f){
      vx = (pxMatrix + 1) << SHIFT;
      vy = Tan*(vx - p.x) + p.y;
      vxo = BLOCK;
      vyo = vxo*Tan;
    } else if(cosf(angle) < -0.001f){
      vx = (pxMatrix << SHIFT) - 0.01f;
      vy = Tan*(vx - p.x) + p.y;
      vxo = -BLOCK;
      vyo = vxo*Tan;
    } else {
      vx = p.x; vy = p.y; checks = MAP_SIZE; dv = 1e6f;
    }
    while(checks < MAP_SIZE){
      mx = (int)vx >> SHIFT;
      my = (int)vy >> SHIFT;
      if(mx >=0 && my >=0 && mx < MAP_SIZE && my < MAP_SIZE && walls[my*MAP_SIZE + mx] > 0){
        checks = MAP_SIZE;
        dv = distf(p.x,p.y,vx,vy);
        textureIndex = walls[my*MAP_SIZE + mx] - 1;
      } else {
        vx += vxo; vy += vyo;
      }
      checks++;
    }

    // horizontal check
    float nx, ny, nxo, nyo, dh;
    if (Tan == 0.0f) Tan = 1e-6f;
    Tan = 1.0f / Tan;
    checks = 0; dh = 1e6f;
    if(sinf(angle) > 0.001f){
      ny = (pyMatrix + 1) << SHIFT;
      nx = p.x + (ny - p.y) * Tan;
      nyo = BLOCK;
      nxo = nyo * Tan;
    } else if(sinf(angle) < -0.001f){
      ny = (pyMatrix << SHIFT) - 0.01f;
      nx = p.x + (ny - p.y) * Tan;
      nyo = -BLOCK;
      nxo = nyo * Tan;
    } else {
      nx = p.x; ny = p.y; checks = MAP_SIZE;
    }
    while(checks < MAP_SIZE){
      mx = (int)nx >> SHIFT;
      my = (int)ny >> SHIFT;
      if(mx >=0 && my >=0 && mx < MAP_SIZE && my < MAP_SIZE && walls[my*MAP_SIZE + mx] > 0){
        checks = MAP_SIZE;
        dh = distf(p.x,p.y,nx,ny);
        textureIndex = walls[my*MAP_SIZE + mx] - 1;
      } else {
        nx += nxo; ny += nyo;
      }
      checks++;
    }

    float shade = 1.0f;
    if(dv < dh){
      shade = 0.5f;
      nx = vx; ny = vy; dh = dv;
    }

    // store depth for sprite z-buffer
    depth[r] = dh;

    // draw ray as a line on minimap overlay (optional)
    // convert to int coords
    // (we won't draw every ray on canvas to save time)

    // Fisheye correction
    float ca = p.a - angle;
    dh = dh * cosf(ca);

    int lineH = (BLOCK * HEIGHT) / (dh > 0.001f ? dh : 0.001f);
    float y_step = 32.0f / (float)lineH;
    float tyo = 0;
    if(lineH > HEIGHT){ tyo = (lineH - HEIGHT) / 2.0f; lineH = HEIGHT; }
    int lineOff = (HEIGHT/2) - (lineH >> 1);

    float ty = tyo * y_step + textureIndex * 32.0f;
    float tx;
    if(shade == 1.0f) { tx = (int)(nx*2.0f) % 32; if(angle < PI) tx = 31 - tx; }
    else { tx = (int)(ny*2.0f) % 32; if(angle > P2 && angle < P3) tx = 31 - tx; }

    // draw vertical slice into canvas at column r (scale to WIDTH)
    int screenX = r; // r goes 0..RAYS-1, RAYS==WIDTH
    for(int j=0; j < lineH; j++){
      int tyi = (int)ty;
      int txi = (int)tx & 31;
      int tidx = tyi * 32 + txi;
      // safety checks
      if(tidx >= 0 && tidx < (32*32*16)) {
        uint16_t pixel = textures[tidx];
        // apply shade by choosing brighter/darker variant is not easy on 565:
        // We'll simply darken by mixing with black: create a cheaper lookup by shifting components
        if(shade < 1.0f){
          // crude darken
          uint16_t p = pixel;
          uint8_t r5 = (p >> 11) & 0x1F;
          uint8_t g6 = (p >> 5) & 0x3F;
          uint8_t b5 = p & 0x1F;
          r5 = (uint8_t)(r5 * shade);
          g6 = (uint8_t)(g6 * shade);
          b5 = (uint8_t)(b5 * shade);
          pixel = (r5 << 11) | (g6 << 5) | b5;
        }
        plot(screenX, lineOff + j, pixel);
      }
      ty += y_step;
    }

    // floor + ceiling (simple textured floor/ceiling)
    float rayDirX = cosf(angle);
    float rayDirY = sinf(angle);
    for(int y = lineOff + lineH; y < HEIGHT; y++){
      float dy = (y - HEIGHT/2.0f);
      if(fabsf(dy) < 1e-3f) dy = 1e-3f;
      float rowDist = (0.5f * HEIGHT) / dy;
      rowDist = rowDist / cosf(p.a - angle);
      float floorX = p.x/16.0f + rowDist * rayDirX;
      float floorY = p.y/16.0f + rowDist * rayDirY;
      int tx = ((int)(floorX * 32.0f)) & 31;
      int ty2 = (((int)(floorY * 32.0f)) & 31) + 64;
      int idx = ty2 * 32 + tx;
      if(idx >= 0 && idx < (32*32*16)){
        uint16_t pixel = textures[idx];
        // darken floor
        uint16_t dark = (((pixel >> 11) & 0x1F) >> 1) << 11;
        dark |= ((((pixel >> 5) & 0x3F) >> 1) << 5);
        dark |= ((pixel & 0x1F) >> 1);
        plot(screenX, y, dark);

        // mirrored ceiling
        pixel = textures[idx + 1024];
        // darken floor
        dark = (((pixel >> 11) & 0x1F) >> 1) << 11;
        dark |= ((((pixel >> 5) & 0x3F) >> 1) << 5);
        dark |= ((pixel & 0x1F) >> 1);
        plot(screenX, HEIGHT - y, dark);
      }
    }

    // next ray
    angle += FOV / (float)RAYS;
    if(angle > 2*PI) angle -= 2*PI;
  }
}

void handleInput(){
  uint8_t x = CONTROL_PACKET.joyX;
  uint8_t y = CONTROL_PACKET.joyY;

  if(y > 150){
    p.x -= p.dx;
    p.y -= p.dy;
  }
  else if(y < 90){
    p.x += p.dx;
    p.y += p.dy;
  }

  if( x > 150){
    p.a += ANGLE_VELOCITY; if(p.a > 2*PI){p.a -= 2*PI;} p.dx = cos(p.a)*VELOCITY; p.dy = sin(p.a)*VELOCITY;
  }
  else if(x < 90){
    p.a -= ANGLE_VELOCITY; if(p.a < 0){p.a += 2*PI;} p.dx = cos(p.a)*VELOCITY; p.dy = sin(p.a)*VELOCITY;
  }

}

// ---------- Main render ----------
void renderFrame(){
  clearcanvas(); // fondo

  drawRays();      // 3D
  drawEntities();  // sprites (z-buffer with depth[])
  // drawMap2D();     // minimap overlay
  // drawPlayerMinimap();

  // push canvas buffer to tft
  updateTft();
}

// ---------- Input handling ----------
bool paused = false;
unsigned long lastFrame = 0;
const unsigned long FRAME_MS = 33; // ~30 FPS

void initZombiesGame() {
  canvas.fillScreen(0);
  playSongAsyncStart(zombies_freq,zombies_dur, zombies_start,ZOMBIES_NOTES);
  initGame();
  lastFrame = millis();
}

void loopZombiesGame() {
  while(1){
    unsigned long now = millis();
    playSongAsyncUpdate();
    if(now - lastFrame >= FRAME_MS){ 
      lastFrame = now;
      if(!paused){
        handleInput();
        renderFrame();
      } else {
        // aún podemos dibujar overlay de "PAUSA"
        // dibujar texto en canvas y push
        canvas.fillScreen(0);
        canvas.setTextSize(2);
        canvas.setTextColor(0xffff);
        canvas.setCursor(30, 100);
        canvas.print("PAUSED");
        updateTft();
      }
    }

    // lee botones con menos frecuencia para ahorrar CPU
    delay(1);
  }
 
}
