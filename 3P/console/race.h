//#include "race.h"
#include "sprites.h"
#include "tft.h"
#include "packetManager.h"
#include "ServoManager.h"
#include "race_buzzer.h"

// ======================================================= CONFIG
#define ACC_STEER 2
#define TIME_INC 7000
#define MAX_ENEMIES 5

// ======================================================= Pilot STRUCT
struct Pilot{
  float vel;
  float steerVel;
  float x;
  const int y = 190;
  const int w = 26;
  const int h = 48;
};

// ======================================================= ENEMY STRUCT
struct Enemy {
  int x, y;
  float speed;
  SpriteInfo* sprite;
  bool active;
};

Pilot pilot;
Enemy enemies[MAX_ENEMIES];

bool gameOver = false;
bool pauseRace = false;
int score = 0;
int roadOffset = 0;

unsigned long lastMillis = 0;
unsigned long lastSpawn = 0;
unsigned long timeHardener = 0;
unsigned long spawnTime = 4000;

SpriteInfo* getSprite(const char *name) {
  for (int i = 0; i < spriteCount; i++)
    if (strcmp(sprites[i].name, name) == 0)
      return &sprites[i];
  return NULL;
}

void drawSpriteWithChroma(const uint16_t *sheet, SpriteInfo *sp, int dx, int dy, uint16_t chroma) {
  for (uint16_t j=0; j < sp->h; j++)
    for (uint16_t i=0; i < sp->w; i++) {
      uint16_t c = sheet[(sp->y + j) * cars_width + (sp->x + i)];
      if (c != chroma) canvas.drawPixel(dx + i, dy + j, c);
    }
}

bool checkCollision(int x1,int y1,int w1,int h1,int x2,int y2,int w2,int h2) {
  return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

void spawnEnemy() {
  for(int i=0; i < MAX_ENEMIES; i++){
    if(!enemies[i].active){
      enemies[i].active = true;
      enemies[i].sprite = &sprites[random(spriteCount)];
      enemies[i].x = 60 + random(100);
      enemies[i].y = -100;
      enemies[i].speed = 1;
      return;
    }
  }
}

void initRaceGame(){
  pilot.vel = 10;
  pilot.x = 80;
  pilot.steerVel = 0;

  score = 0;
  gameOver = false;
  pauseRace = 0;
  roadOffset = 0;

  // RESET de timers
  lastMillis = millis();
  lastSpawn = millis();
  timeHardener = millis();
  spawnTime = 4000;  // valor inicial

  // RESET COMPLETO de enemigos
  for(int i = 0; i < MAX_ENEMIES; i++){
    enemies[i].active = false;
    enemies[i].x = 0;
    enemies[i].y = -100;
    enemies[i].speed = 1;
    enemies[i].sprite = NULL;
  }

  playSongAsyncStart(racem_freq, racem_dur, racem_start, racem_NOTES);
}

unsigned long lastBack = 0;
unsigned long lastServo = 0;

void loopRaceGame(){
  while(1){
  playSongAsyncUpdate();


  if((CONTROL_PACKET.buttons & 8) && (millis() - lastBack > 500)){
    lastBack  = millis();
    pauseRace = !pauseRace;
  }


  if(pauseRace){ 
      canvas.fillScreen(0);
      canvas.setTextSize(2);
      canvas.setTextColor(0xffff);
      canvas.setCursor(30, 100);
      canvas.print("PAUSED");
      updateTft();
      continue;
  }

  if(CONTROL_PACKET.buttons & 4){
    silenceBuzzer();
    return;
  }

  if (gameOver){
    canvas.fillScreen(0);
    canvas.setCursor(50, 100);
    canvas.setTextColor(0xFFFF);
    canvas.println("CRASH!Score: ");
    canvas.setCursor(50, canvas.getCursorY());
    canvas.print("Score: ");
    canvas.print(score);
    updateTft();
    initRaceGame();
    delay(3000);
  }

  unsigned long now = millis();
  unsigned long elapsed = now - lastMillis;

  if(now - timeHardener > TIME_INC){
    pilot.vel      += 5;  if(pilot.vel > 50) pilot.vel = 50;
    spawnTime  -= 200; if (spawnTime < 200) spawnTime = 200;
    timeHardener = now;
  }

  canvas.fillScreen(0x56e4);


  //Draw Road
  if (elapsed > 16) {
    roadOffset += (pilot.vel * 16) / 100;
    roadOffset %= road_height;
    lastMillis = now;
  }

  for (int y = -50 + roadOffset; y <= 240; y += road_height)
    canvas.drawRGBBitmap(120 - road_width/2, y, road, road_width, road_height);


  //Controles
  int steer = 0;
  // if(CONTROL_PACKET.gyroX < -1 || CONTROL_PACKET.joyX < 100 ) steer = -1;
  // if(CONTROL_PACKET.gyroX > 1 || CONTROL_PACKET.joyX > 160) steer =  1;

  float incl = CONTROL_PACKET.gyroX / 60;

  if(millis() - lastServo > 500){
    rotateServo(90 + (int)CONTROL_PACKET.gyroX);
  }

  pilot.steerVel +=  ACC_STEER * incl;
  pilot.steerVel *= 0.9f;
  pilot.x += pilot.steerVel;
  if(pilot.x < 60) pilot.x = 60;
  if(pilot.x > 180 - pilot.w) pilot.x = 180 - pilot.w;


  // Draw car
  SpriteInfo* car = getSprite("camaro");
  drawSpriteWithChroma(cars, car, pilot.x, pilot.y, 0xf81f);

  for(int i=0;i<MAX_ENEMIES;i++){
    if(enemies[i].active){
      if(elapsed > 16) enemies[i].y += enemies[i].speed + pilot.vel * 0.2;

      if(enemies[i].y > 240){
        enemies[i].active = false;
        score++;
        continue;
      }

      drawSpriteWithChroma(cars, enemies[i].sprite, enemies[i].x, enemies[i].y, 0xf81f);

      if(checkCollision(pilot.x,pilot.y,pilot.w,pilot.h, enemies[i].x,enemies[i].y, enemies[i].sprite->w,enemies[i].sprite->h)){
        gameOver = true;
      }
    }
  }

  if(now - lastSpawn > spawnTime){
    spawnEnemy();
    lastSpawn = now;
  }

  canvas.setCursor(5,5);
  canvas.setTextColor(0xFFFF);
  canvas.print("Score:");
  canvas.print(score);

  updateTft();
  }
}
