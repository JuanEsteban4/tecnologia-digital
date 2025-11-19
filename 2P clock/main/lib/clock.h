#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSans12pt7b.h>

#include "res/gato_sin_fondo.h"
#include "res/fondo_manana.h"
#include "res/fondo_tarde.h"
#include "res/fondo_noche.h"

#include "res/gato_sec.h"   // 150x92
#include "res/mano_min.h"   // 99x76
#include "res/mano_hor.h"   // 99x76 (reflejada)


#define TFT_DC    6
#define TFT_RST   5
#define TFT_CS   10
#define VCC 7 //Needed for vcc for screen
#define TFT_W 240
#define TFT_H 240
#define IMG_W 150
#define IMG_H 150
#define AMPLITUDE 8
#define PERIOD 3000

static Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
static GFXcanvas16 canvas(TFT_W, TFT_H);

static unsigned long startTime;
tm currentTimeTM;
uint8_t clockDisplayMode = 0; // 0 digital — 1 date — 2 analog

// -------------------- UTIL --------------------------
static inline uint16_t rgb565(uint8_t r,uint8_t g,uint8_t b){
  return ((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3);
}

static void drawRotatedScaledBitmap(
    int cx, int cy,
    const unsigned short *bitmap,
    int w, int h,
    float angleDeg,
    int pivotX, int pivotY,
    float scale
) {
  float angle = angleDeg * 0.017453292519943295f;
  float cosA = cos(angle);
  float sinA = sin(angle);

  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      float xo = (x - pivotX) * scale;
      float yo = (y - pivotY) * scale;

      int xr = (int)round(xo * cosA - yo * sinA);
      int yr = (int)round(xo * sinA + yo * cosA);

      int px = cx + xr;
      int py = cy + yr;

      if (px < 0 || px >= TFT_W || py < 0 || py >= TFT_H) continue;

      uint16_t color = pgm_read_word(&bitmap[y * w + x]);
      if (color != 0x0000) {
        canvas.drawPixel(px, py, color);
      }
    }
  }
}

// -------------------- RTC LOCAL ---------------------
static void updateLocalTime(){
  struct tm info;
  if (getLocalTime(&info)) currentTimeTM = info;
}

// -------------------- SYNC NTP -----------------------
static void syncNTP(){
  configTime(-18000, 0, "pool.ntp.org"); // Colombia GMT-5
  time_t now = time(nullptr);
  unsigned long t0 = millis();
  while (now < 100000 && millis() - t0 < 6000){
    delay(200);
    now = time(nullptr);
  }
}

// ==================== API ===========================
void setClockMode(uint8_t mode){
  clockDisplayMode = mode;
}

static void applyClockToSystem(){
  time_t newTime = mktime(&currentTimeTM);
  struct timeval now = { .tv_sec = newTime };
  settimeofday(&now, nullptr);
}

void setClockTime(int h,int m,int s){
  currentTimeTM.tm_hour = h;
  currentTimeTM.tm_min  = m;
  currentTimeTM.tm_sec  = s;
  applyClockToSystem();
}

void setClockDate(int d,int m,int y){
  currentTimeTM.tm_mday = d;
  currentTimeTM.tm_mon  = m - 1;
  currentTimeTM.tm_year = y - 1900;
  applyClockToSystem();
}

// =================== INIT ===========================
void setupClock(const char* ssid,const char* pass){
  tft.init(240,240,SPI_MODE3);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);
  canvas.setTextWrap(false);

  startTime = millis();

  //Needed for vcc for screen

  pinMode(VCC, OUTPUT);
  digitalWrite(VCC,HIGH);
  // Leer RTC interno al arrancar
  time_t now = time(nullptr);
  localtime_r(&now, &currentTimeTM);

  // Intentar WiFi solo si se suministra SSID
  if(ssid && pass){
    WiFi.begin(ssid, pass);
    unsigned long t0 = millis();
    while(WiFi.status()!=WL_CONNECTED && millis() - t0 < 8000) delay(200);
    if(WiFi.status()==WL_CONNECTED){
      syncNTP();
      updateLocalTime();
    }else{
       setClockTime(15,10,0);
       setClockDate(18,11,2025);
    }
  }
}

// ==================== DRAW MODES =====================
static void drawDigital(){
  canvas.setFont(&FreeSans12pt7b);
  canvas.setTextSize(2);

  char buf[16];
  int h = currentTimeTM.tm_hour % 12; if(h == 0) h = 12;

  bool blink = currentTimeTM.tm_sec % 2;
  sprintf(buf, blink ? "%02d:%02d %s" : "%02d %02d %s",
        h, currentTimeTM.tm_min,
        currentTimeTM.tm_hour >= 12 ? "PM" : "AM");

  int16_t x1,y1; uint16_t w,hg;
  canvas.getTextBounds(buf,0,0,&x1,&y1,&w,&hg);
  int tx = (TFT_W - w) / 2;
  int ty = (TFT_H + hg) / 2 - 40;

  uint16_t glow = rgb565(138,43,226);
  uint16_t mainC= rgb565(255,105,180);

  canvas.setTextColor(glow);
  for(int dx=-2;dx<=2;dx+=2)
    for(int dy=-2;dy<=2;dy+=2){
      canvas.setCursor(tx+dx,ty+dy);
      canvas.print(buf);
    }

  canvas.setTextColor(mainC);
  canvas.setCursor(tx,ty);
  canvas.print(buf);
}

static void drawDate(){
  canvas.setFont(&FreeSans12pt7b);
  canvas.setTextSize(2);

  char buf[16];
  sprintf(buf, "%02d/%02d/%02d",
          currentTimeTM.tm_mday, currentTimeTM.tm_mon + 1,
          (currentTimeTM.tm_year + 1900) % 100);

  int16_t x1,y1; uint16_t w,hg;
  canvas.getTextBounds(buf,0,0,&x1,&y1,&w,&hg);
  int tx = (TFT_W - w) / 2;
  int ty = (TFT_H + hg) / 2 - 40;

  uint16_t glow = rgb565(138,43,226);
  uint16_t mainC= rgb565(255,105,180);

  canvas.setTextColor(glow);
  for(int dx=-2;dx<=2;dx+=2)
    for(int dy=-2;dy<=2;dy+=2){
      canvas.setCursor(tx+dx,ty+dy);
      canvas.print(buf);
    }

  canvas.setTextColor(mainC);
  canvas.setCursor(tx,ty);
  canvas.print(buf);
}

static void drawAnalog(){
  int cx = TFT_W / 2;
  int cy = TFT_H / 2;
  int r  = 90;

  uint16_t mainC = rgb565(255,105,180);
  uint16_t glow  = rgb565(138,43,226);

  canvas.drawCircle(cx, cy, r, mainC);
  canvas.drawCircle(cx, cy, r-1, glow);

  for(int i=0;i<12;i++){
    float ang = i * 30 * DEG_TO_RAD;
    int x1 = cx + cos(ang) * (r - 12);
    int y1 = cy + sin(ang) * (r - 12);
    int x2 = cx + cos(ang) * (r - 3);
    int y2 = cy + sin(ang) * (r - 3);
    canvas.drawLine(x1,y1,x2,y2,mainC);
  }

  float angSec = (currentTimeTM.tm_sec * 6 - 90) * DEG_TO_RAD;
  float angMin = ((currentTimeTM.tm_min + currentTimeTM.tm_sec/60.0) * 6 - 90) * DEG_TO_RAD;
  float angHr  = ((currentTimeTM.tm_hour % 12 + currentTimeTM.tm_min/60.0) * 30 - 90) * DEG_TO_RAD;

  canvas.drawLine(cx,cy, cx + cos(angSec)*(r-14), cy + sin(angSec)*(r-14), rgb565(255,0,0));
  canvas.drawLine(cx,cy, cx + cos(angMin)*(r-24), cy + sin(angMin)*(r-24), mainC);
  canvas.drawLine(cx,cy, cx + cos(angHr )*(r-42), cy + sin(angHr )*(r-42), glow);
  canvas.fillCircle(cx,cy,4, mainC);
}

// =================== UPDATE ==========================
void updateClock(){
  updateLocalTime();

  unsigned long now = millis();
  float phase = (now - startTime) * (2.0 * PI / PERIOD);
  int lev = round(AMPLITUDE * sin(phase));

  const uint16_t* fondo =
    (currentTimeTM.tm_hour >= 6 && currentTimeTM.tm_hour < 12) ? fondo_manana :
    (currentTimeTM.tm_hour >= 12 && currentTimeTM.tm_hour < 18) ? fondo_tarde :
                                                                  fondo_noche;

  canvas.fillScreen(ST77XX_BLACK);

  for(int y=0;y<TFT_H;y++){
    int sy=(y*IMG_H)/TFT_H;
    for(int x=0;x<TFT_W;x++){
      int sx=(x*IMG_W)/TFT_W;
      uint16_t p=pgm_read_word(&fondo[sy*IMG_W + sx]);
      canvas.drawPixel(x,y,p);
    }
  }

  int gx = (TFT_W-IMG_W)/2;
  int gy = (TFT_H-IMG_H)/2 + lev;

  for(int j=0;j<IMG_H;j++)
    for(int i=0;i<IMG_W;i++){
      uint16_t p = pgm_read_word(&gato_sin_fondo[j*IMG_W + i]);
      if(p != 0x0000) canvas.drawPixel(gx+i, gy+j, p);
    }

  if      (clockDisplayMode == 0) drawDigital();
  else if (clockDisplayMode == 1) drawDate();
  else                            drawAnalog();

  tft.drawRGBBitmap(0,0,canvas.getBuffer(),TFT_W,TFT_H);
}
