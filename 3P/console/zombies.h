/* Raycast ESP32-S3 + ST7789 (240x240) using Adafruit_GFX + GFXcanvas16
   Controls:
   - Joystick analog X/Y (0..255 expected). Mapped to rotation (X) and forward/back (Y).
   - BTN_PAUSE, BTN_BACK, BTN_A, BTN_B (digital buttons, pull-down assumed)
   New Features:
   - Wall collision detection
   - Zombie AI that moves toward player
   - Shooting mechanics to kill zombies
*/

#pragma once
#include "tft.h"
#include "sprites.h"
#include "packetManager.h"
#include "buzzerManager.h"
#include "zombies_buzzer.h"

// ---------- Config ----------
#define PI 3.14159265359f
#define P2 1.57079632679f
#define P3 4.71238898038f
#define WIDTH 240
#define HEIGHT 240
#define VELOCITY 3.0f
#define ANGLE_VELOCITY 0.139626f
#define MAP_SIZE 15
#define BLOCK 16
#define SHIFT 4
#define FOV 1.0472f
#define RAYS 240
#define MAX_ZOMBIES 5
#define ZOMBIE_SPEED 1.0f
#define ZOMBIE_ATTACK_RANGE 20.0f
#define SHOOT_COOLDOWN 500  // ms
#define ZOMBIE_HIT_RANGE 10.0f
#define ZOMBIE_SPAWN_TIME 5000  // ms between spawns

// ---------- Game Data ----------
struct Player {
    float x, y, a;
    float dx, dy;
    int health = 100;
    int score = 0;
    bool shooting = false;
    unsigned long lastShotTime = 0;
} p;

struct Entity {
    float x, y, z;
    bool active = false;
    int health = 3;
    float moveTimer = 0;
    unsigned long lastAttackTime = 0;
    int textureIndex = 0;  // Zombie texture
} entities[MAX_ZOMBIES];

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

bool gameOverZ = 0;

// ---------- Utils ----------
inline float distf(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    return sqrtf(dx * dx + dy * dy);
}

inline uint16_t rgb565_from_texture(uint16_t c) { return c; }

void clearcanvas(uint16_t color = 0x0000) {
    canvas.fillScreen(color);
}

inline void plot(int x, int y, uint16_t color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
        canvas.drawPixel(x, y, color);
}

// ---------- Collision Detection ----------
bool checkWallCollision(float x, float y, float radius = 4.0f) {
    // Check map boundaries
    if (x < radius || x > WIDTH - radius || y < radius || y > HEIGHT - radius)
        return true;
    
    // Check walls in map
    int mx = (int)x >> SHIFT;
    int my = (int)y >> SHIFT;
    
    // Check 4 surrounding cells for safety
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int checkX = mx + dx;
            int checkY = my + dy;
            
            if (checkX >= 0 && checkX < MAP_SIZE && checkY >= 0 && checkY < MAP_SIZE) {
                if (walls[checkY * MAP_SIZE + checkX] > 0) {
                    // Calculate distance to wall cell
                    float wallX = checkX * BLOCK + BLOCK / 2;
                    float wallY = checkY * BLOCK + BLOCK / 2;
                    if (distf(x, y, wallX, wallY) < radius + BLOCK / 2)
                        return true;
                }
            }
        }
    }
    return false;
}

// ---------- Zombie Management ----------
int findFreeZombieSlot() {
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (!entities[i].active)
            return i;
    }
    return -1;
}

void orderZombies() {
    // Bubble-sort/simple stable sort por distancia al jugador.
    // Zombies activos quedan al inicio ordenados por distancia (más cerca primero).
    // Zombies inactivos se consideran "muy lejos" y quedan al final.
    for (int pass = 0; pass < MAX_ZOMBIES - 1; pass++) {
        bool swapped = false;
        for (int j = 0; j < MAX_ZOMBIES - 1 - pass; j++) {
            // distancia al cuadrado (evita sqrt)
            float d1 = entities[j].active ? (p.x - entities[j].x)*(p.x - entities[j].x) + (p.y - entities[j].y)*(p.y - entities[j].y) 
                                          : 1e12f; // inactivo => muy lejos
            float d2 = entities[j+1].active ? (p.x - entities[j+1].x)*(p.x - entities[j+1].x) + (p.y - entities[j+1].y)*(p.y - entities[j+1].y) 
                                            : 1e12f;

            // Queremos más cerca primero -> si d1 > d2 entonces intercambiamos
            if (d1 > d2) {
                Entity tmp = entities[j];
                entities[j] = entities[j+1];
                entities[j+1] = tmp;
                swapped = true;
            }
        }
        if (!swapped) break; // ya ordenado
    }
}


void spawnZombie() {
    int slot = findFreeZombieSlot();
    if (slot == -1) return;
    
    // Try to spawn away from player
    float angle = random(0, 360) * PI / 180.0f;
    float distance = random(50, 100);
    
    Entity& z = entities[slot];
    z.x = p.x + cosf(angle) * distance;
    z.y = p.y + sinf(angle) * distance;
    z.z = 120;
    z.active = true;
    z.health = 3;
    z.moveTimer = 0;
    z.lastAttackTime = 0;
    z.textureIndex = (slot % 4);  // Different zombie types
}

void updateZombies() {
    orderZombies();
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        Entity& z = entities[i];
        if (!z.active) continue;
        
        // Move toward player
        float dx = p.x - z.x;
        float dy = p.y - z.y;
        float dist = sqrtf(dx * dx + dy * dy);
        
        if (dist > 0.1f) {
            // Normalize direction
            dx /= dist;
            dy /= dist;
            
            // Move zombie
            float newX = z.x + dx * ZOMBIE_SPEED;
            float newY = z.y + dy * ZOMBIE_SPEED;
            
            // Check collision with walls for zombie
            // if (!checkWallCollision(newX, newY, 6.0f)) {
                 z.x = newX;
                 z.y = newY;
            // }
            
            // Attack player if close enough
            if (dist < ZOMBIE_ATTACK_RANGE && (millis() - z.lastAttackTime > 1000)) {
                p.health -= 10;
                z.lastAttackTime = millis();
                
                if (p.health <= 0) {
                    // Game over
                    p.health = 0;
                    gameOverZ = 1;
                }
            }
        }
        
        // Random movement variation
        z.moveTimer += 0.1f;
        if (z.moveTimer > 2.0f) {
            // Small random direction changes
            float randomAngle = (random(-30, 30) * PI / 180.0f);
            float tempX = dx * cosf(randomAngle) - dy * sinf(randomAngle);
            float tempY = dx * sinf(randomAngle) + dy * cosf(randomAngle);
            dx = tempX;
            dy = tempY;
            z.moveTimer = 0;
        }
    }
}

void shoot() {
    unsigned long currentTime = millis();
    if (currentTime - p.lastShotTime < SHOOT_COOLDOWN) return;
    
    p.shooting = true;
    p.lastShotTime = currentTime;
    
    // Play shoot sound
    //playTone(500, 50);
    
    // Check for zombie hits
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        Entity& z = entities[i];
        if (!z.active) continue;
        
        // Check if zombie is in front of player
        float dx = z.x - p.x;
        float dy = z.y - p.y;
        float dist = sqrtf(dx * dx + dy * dy);
        
        // Normalize
        if (dist > 0.1f) {
            dx /= dist;
            dy /= dist;
            
            // Dot product with player direction
            float dot = dx * cosf(p.a) + dy * sinf(p.a);
            
            // If zombie is in front 
            if (dot > 0.95f && dist < 100.0f) {
                z.health--;
                
                if (z.health <= 0) {
                    // Zombie killed
                    z.active = false;
                    p.score += 100;
                    
                    // Play death sound
                    //playTone(150, 200);
                } else {
                    // Play hit sound
                    //playTone(200, 100);
                }
                break;  // Only hit one zombie per shot
            }
        }
    }
}

// ---------- Drawing ----------
void drawHUD() {
    // Health bar
    int barWidth = 100;
    int barHeight = 8;
    int barX = 5;
    int barY = HEIGHT - 15;
    
    // Background
    canvas.fillRect(barX, barY, barWidth, barHeight, 0x4208);
    
    // Health fill
    int healthWidth = (p.health * barWidth) / 100;
    canvas.fillRect(barX, barY, healthWidth, barHeight, 0x07E0);  // Green
    
    // Health bar border
    canvas.drawRect(barX, barY, barWidth, barHeight, 0xFFFF);
    
    // Score
    canvas.setCursor(5, HEIGHT - 30);
    canvas.setTextColor(0xFFFF);
    canvas.setTextSize(1);
    canvas.print("Score: ");
    canvas.print(p.score);
    
    // Ammo indicator
    canvas.setCursor(WIDTH - 50, HEIGHT - 15);
    canvas.print("SHOT");
    if (millis() - p.lastShotTime < SHOOT_COOLDOWN) {
        // Cooldown indicator
        int cooldownWidth = 40 * (SHOOT_COOLDOWN - (millis() - p.lastShotTime)) / SHOOT_COOLDOWN;
        canvas.fillRect(WIDTH - 45, HEIGHT - 10, cooldownWidth, 3, 0xF800);
    }
}

// ---------- Draw Entities (sprites) ----------
float f = (WIDTH / 2) / tanf(FOV / 2.0f);
void drawEntities() {
    for (int i = MAX_ZOMBIES - 1; i >= 0; i--) {
        Entity& e = entities[i];
        if (!e.active) continue;
        
        float dx = e.x - p.x;
        float dy = e.y - p.y;
        float dz = e.z - (HEIGHT / 2);
        
        float Cos = cosf(p.a), Sin = sinf(p.a);
        float rx = dx * Cos + dy * Sin;
        float ry = -dx * Sin + dy * Cos;
        if (rx <= 8.0f) continue;
        
        
        int screenX = (int)((ry * f / rx) + (WIDTH / 2));
        int screenY = (int)((dz * f / rx) + (HEIGHT / 2));
        
        float spriteConstant = 3000.0f;
        int scale = (int)(spriteConstant / rx);
        if (scale < 4) scale = 4;
        
        int offset = e.textureIndex * 32 * 32;
        
        float t_x = 0.0f;
        float t_x_step = 32.0f / (float)scale;
        float t_y_step = 32.0f / (float)scale;
        
        for (int sx = screenX - scale / 2; sx < screenX + scale / 2; sx++) {
            float t_y = 0.0f;
            for (int sy = screenY - scale / 2; sy < screenY + scale / 2; sy++) {
                if (sx >= 0 && sx < WIDTH && sy >= 0 && sy < HEIGHT && rx < depth[sx]) {
                    int ti = ((int)t_y) * 32 + (int)t_x + offset;
                    if (ti >= 0 && ti < (32 * 32 * 16)) {
                        uint16_t pixel = zombies[ti];
                        if (pixel != 0xF81F) {
                            // Flash white if zombie was recently hit
                            if (e.health < 3 && (millis() % 500 < 100)) {
                                pixel = 0xFFFF;
                            }
                            plot(sx, sy, rgb565_from_texture(pixel));
                        }
                    }
                }
                t_y += t_y_step;
            }
            t_x += t_x_step;
        }
    }
}

// ---------- Initialize Game ----------
void initGame() {
    p.x = 120;
    p.y = 120;
    p.a = -PI / 2;
    p.dx = cosf(p.a) * VELOCITY;
    p.dy = sinf(p.a) * VELOCITY;
    p.health = 100;
    p.score = 0;
    
    // Initialize all zombies as inactive
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        entities[i].active = false;
    }
    
    gameOverZ = 0;
    // Spawn initial zombies
    spawnZombie();
    spawnZombie();
}

// ---------- Raycaster (unchanged except for depth buffer usage) ----------
// ---------- Raycaster ----------
void drawRays(){
    int pxMatrix = ((int)p.x) >> SHIFT;
    int pyMatrix = ((int)p.y) >> SHIFT;

    float angle = p.a - FOV/2.0f;

    for(int r=0; r<RAYS; r+=2){

        float Tan = tanf(angle);
        float vx, vy, vxo, vyo, dv;
        dv = 1e6f;
        int checks = 0;
        int mx,my;
        int textureIndex = 0;

        if(cosf(angle) > 0.001f){
            vx = (pxMatrix + 1) << SHIFT;
            vy = Tan*(vx - p.x) + p.y;
            vxo = BLOCK;
            vyo = vxo*Tan;
        }
        else if(cosf(angle) < -0.001f){
            vx = (pxMatrix << SHIFT) - 0.01f;
            vy = Tan*(vx - p.x) + p.y;
            vxo = -BLOCK;
            vyo = vxo*Tan;
        }
        else {
            vx = p.x;
            vy = p.y;
            checks = MAP_SIZE;
            dv = 1e6f;
        }

        while(checks < MAP_SIZE){
            mx = (int)vx >> SHIFT;
            my = (int)vy >> SHIFT;
            if(mx>=0 && my>=0 && mx<MAP_SIZE && my<MAP_SIZE && walls[my*MAP_SIZE + mx] > 0){
                checks = MAP_SIZE;
                dv = distf(p.x,p.y,vx,vy);
                textureIndex = walls[my*MAP_SIZE + mx] - 1;
            } else {
                vx += vxo;
                vy += vyo;
            }
            checks++;
        }

        float nx, ny, nxo, nyo, dh;
        if (Tan == 0.0f) Tan = 1e-6f;
        Tan = 1.0f / Tan;
        checks = 0;
        dh = 1e6f;

        if(sinf(angle) > 0.001f){
            ny = (pyMatrix + 1) << SHIFT;
            nx = p.x + (ny - p.y) * Tan;
            nyo = BLOCK;
            nxo = nyo * Tan;
        }
        else if(sinf(angle) < -0.001f){
            ny = (pyMatrix << SHIFT) - 0.01f;
            nx = p.x + (ny - p.y) * Tan;
            nyo = -BLOCK;
            nxo = nyo * Tan;
        }
        else {
            nx = p.x;
            ny = p.y;
            checks = MAP_SIZE;
        }

        while(checks < MAP_SIZE){
            mx = (int)nx >> SHIFT;
            my = (int)ny >> SHIFT;
            if(mx>=0 && my>=0 && mx<MAP_SIZE && my<MAP_SIZE && walls[my*MAP_SIZE + mx] > 0){
                checks = MAP_SIZE;
                dh = distf(p.x,p.y,nx,ny);
                textureIndex = walls[my*MAP_SIZE + mx] - 1;
            } else {
                nx += nxo;
                ny += nyo;
            }
            checks++;
        }

        float shade = 1.0f;
        if(dv < dh){
            shade = 0.5f;
            nx = vx;
            ny = vy;
            dh = dv;
        }

        depth[r] = dh;

        float ca = p.a - angle;
        dh = dh * cosf(ca);

        int lineH = (BLOCK * HEIGHT) / (dh > 0.001f ? dh : 0.001f);
        float y_step = 32.0f / (float)lineH;
        float tyo = 0;

        if(lineH > HEIGHT){
            tyo = (lineH - HEIGHT) / 2.0f;
            lineH = HEIGHT;
        }

        int lineOff = (HEIGHT/2) - (lineH >> 1);

        float ty = tyo * y_step + textureIndex * 32.0f;
        float tx;

        if(shade == 1.0f) {
            tx = (int)(nx*2.0f) % 32;
            if(angle < PI) tx = 31 - tx;
        }
        else {
            tx = (int)(ny*2.0f) % 32;
            if(angle > P2 && angle < P3) tx = 31 - tx;
        }

        int screenX = r;

        for(int j=0; j<lineH; j++){
            int tyi = (int)ty;
            int txi = ((int)tx) & 31;
            int tidx = tyi*32 + txi;

            if(tidx >= 0 && tidx < (32*32*16)){
                uint16_t pixel = textures[tidx];
                if(shade < 1.0f){
                    uint16_t p = pixel;
                    uint8_t r5 = (p>>11)&0x1F;
                    uint8_t g6 = (p>>5)&0x3F;
                    uint8_t b5 = p & 0x1F;
                    r5 = (uint8_t)(r5 * shade);
                    g6 = (uint8_t)(g6 * shade);
                    b5 = (uint8_t)(b5 * shade);
                    pixel = (r5<<11)|(g6<<5)|b5;
                }
                plot(screenX, lineOff + j, pixel);
                plot(screenX + 1, lineOff + j, pixel);

            }
            ty += y_step;
        }

        float rayDirX = cosf(angle);
        float rayDirY = sinf(angle);

        for(int y=lineOff+lineH; y<HEIGHT; y++){
            float dy = (y - HEIGHT/2.0f);
            if(fabsf(dy) < 1e-3f) dy = 1e-3f;

            float rowDist = (0.5f * HEIGHT) / dy;
            rowDist /= cosf(p.a - angle);

            float floorX = p.x/16.0f + rowDist * rayDirX;
            float floorY = p.y/16.0f + rowDist * rayDirY;

            int tx = ((int)(floorX * 32.0f)) & 31;
            int ty2 = (((int)(floorY * 32.0f)) & 31) + 64;

            int idx = ty2 * 32 + tx;

            if(idx >= 0 && idx < (32*32*16)){
                uint16_t pixel = textures[idx];
                uint16_t dark = (((pixel>>11)&0x1F)>>1)<<11;
                dark |= ((((pixel>>5)&0x3F)>>1)<<5);
                dark |= ((pixel&0x1F)>>1);
                plot(screenX, y, dark);
                plot(screenX + 1, y, dark);



                pixel = textures[idx+1024];
                dark = (((pixel>>11)&0x1F)>>1)<<11;
                dark |= ((((pixel>>5)&0x3F)>>1)<<5);
                dark |= ((pixel&0x1F)>>1);
                plot(screenX, HEIGHT - y, dark);
                plot(screenX + 1, HEIGHT - y, dark);

            }
        }

        angle += (FOV/(float)RAYS)*2;
        if(angle > 2*PI) angle -= 2*PI;
    }
}
// ---------- Main Loop ----------
bool paused = false;
bool exitGame = false;
unsigned long lastFrame = 0;
unsigned long lastSpawnTime = 0;
const unsigned long FRAME_MS = 33;

// ---------- Input ----------
unsigned long lastBackk = 0;
void handleInput() {
    uint8_t x = CONTROL_PACKET.joyX;
    uint8_t y = CONTROL_PACKET.joyY;
    
    bool A = CONTROL_PACKET.buttons & 1;
    bool B = CONTROL_PACKET.buttons & 2;
    bool Bpause = CONTROL_PACKET.buttons & 8;
    bool Bback = CONTROL_PACKET.buttons & 4;
    
    // Forward/backward movement with collision
    if (y < 90) {  // Forward
        float newX = p.x - p.dx;
        float newY = p.y - p.dy;
        if (!checkWallCollision(newX, newY)) {
            p.x = newX;
            p.y = newY;
        }
    } else if (y > 150) {  // Backward
        float newX = p.x + p.dx;
        float newY = p.y + p.dy;
        if (!checkWallCollision(newX, newY)) {
            p.x = newX;
            p.y = newY;
        }
    }
    
    // Rotation
    if (x > 150) {
        p.a += ANGLE_VELOCITY;
        if (p.a > 2 * PI) p.a -= 2 * PI;
        p.dx = cos(p.a) * VELOCITY;
        p.dy = sin(p.a) * VELOCITY;
    } else if (x < 90) {
        p.a -= ANGLE_VELOCITY;
        if (p.a < 0) p.a += 2 * PI;
        p.dx = cos(p.a) * VELOCITY;
        p.dy = sin(p.a) * VELOCITY;
    }
    
    // Shooting (A button)
    if (A) {
        shoot();
    }

    if (B) {
      p.a = (p.a + PI);
      if (p.a > 2 * PI) p.a -= 2 * PI;
    }
    
    if (Bpause && millis() - lastBackk > 500 ) {
        lastBackk = millis();
        paused = !paused;
    }
    
    if (Bback) {
        exitGame = true;
    }
}

// ---------- Render ----------
void renderFrame() {
    clearcanvas();
    drawRays();
    drawEntities();
    drawHUD();
    
    // Draw crosshair
    if (p.shooting) {
        // Flash white when shooting
        canvas.drawPixel(WIDTH / 2, HEIGHT / 2, 0xFFFF);
        canvas.drawPixel(WIDTH / 2 + 1, HEIGHT / 2, 0xFFFF);
        canvas.drawPixel(WIDTH / 2 - 1, HEIGHT / 2, 0xFFFF);
        canvas.drawPixel(WIDTH / 2, HEIGHT / 2 + 1, 0xFFFF);
        canvas.drawPixel(WIDTH / 2, HEIGHT / 2 - 1, 0xFFFF);
        p.shooting = false;
    } else {
        // Normal crosshair
        canvas.drawPixel(WIDTH / 2, HEIGHT / 2, 0xF800);  // Red center
    }
    
    updateTft();
}

// ---------- Game Loop ----------
void initZombiesGame() {
    canvas.fillScreen(0);
    playSongAsyncStart(zombies_freq, zombies_dur, zombies_start, ZOMBIES_NOTES);
    initGame();
    lastFrame = millis();
    lastSpawnTime = millis();
}

void loopZombiesGame() {
    while (1) {
        if (exitGame) {
          exitGame = 0;
          silenceBuzzer();
          break;
        }
        
        unsigned long now = millis();
        playSongAsyncUpdate();
        
        if (now - lastFrame >= FRAME_MS) {
            if(gameOverZ){
              silenceBuzzer();
              canvas.fillScreen(0);
              canvas.setCursor(50, 100);
              canvas.setTextColor(0xFFFF);
              canvas.println("You're death - Score: ");
              canvas.setCursor(50, canvas.getCursorY());
              canvas.print("Score: ");
              canvas.print(p.score);
              updateTft();
              initGame();
              delay(5000);
              playSongAsyncStart(zombies_freq, zombies_dur, zombies_start, ZOMBIES_NOTES);
            }


            handleInput();
            if (!paused) {
                updateZombies();
                
                // Spawn new zombies periodically
                if (now - lastSpawnTime > ZOMBIE_SPAWN_TIME) {
                    spawnZombie();
                    lastSpawnTime = now;
                }
                
                renderFrame();
            } else {
                canvas.fillScreen(0);
                canvas.setTextSize(2);
                canvas.setTextColor(0xFFFF);
                canvas.setCursor(30, 100);
                canvas.print("PAUSED");
                updateTft();
            }
            lastFrame = now;
        }
        delay(1);
    }
}