/*******************************************************************************************
*
*   Demon FPS - Beautiful UI Edition
*
********************************************************************************************/

#include "raymob.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

//------------------------------------------------------------------------------------
// Game Constants
//------------------------------------------------------------------------------------
#define MAP_SIZE 16
#define FOV 60.0f
#define MAX_DEMONS 5
#define MAX_PARTICLES 30
#define SPAWN_INTERVAL 8.0f

typedef enum { MENU, PLAYING, PAUSED, GAME_OVER } GameState;

//------------------------------------------------------------------------------------
// Types and Structures
//------------------------------------------------------------------------------------
typedef struct {
    float x, y, vx, vy, angle;
    int health, ammo, score;
} Player;

typedef struct {
    float x, y, vx, vy, hp, speed, distance, angle, spawnX, spawnY;
    int type;
    bool alive;
} Demon;

typedef struct {
    Vector2 pos, stickPos;
    float radius, deadZone;
    bool active;
    int touchId;
} Joystick;

typedef struct {
    Rectangle rect;
    const char* label;
    bool pressed, wasPressed;
    Color bgColor, textColor, glowColor;
    float glowIntensity;
    int touchId;
} Button;

typedef struct {
    Vector2 pos, vel;
    float life;
    Color color;
} Particle;

//------------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------------
Player player = { 2.5f, 2.5f, 0, 0, 0, 100, 30, 0 };
Demon demons[MAX_DEMONS];
Particle particles[MAX_PARTICLES];
int demonCount = 0, particleCount = 0;
float lastShot = 0, lastSpawn = 0;
GameState gameState = MENU;
float lookSensitivity = 1.0f, masterVolume = 0.7f;
bool cameraActive = false;
int cameraTouchId = -1;
Vector2 cameraStartPos = {0};
float cameraStartAngle = 0;

// Physics
const float ACCEL = 15.0f, FRICTION = 8.0f, MAX_SPEED = 4.0f;
const float DEMON_ACCEL = 8.0f, DEMON_FRICTION = 4.0f;

// BIGGER UI Elements
Joystick moveJoystick = { {0}, 0, {0}, 180, 0.15f, false, -1 };  // Bigger radius
Button fireBtn = { {0}, "🔥", false, false, {0}, {0}, {0}, 0, -1 };
Button reloadBtn = { {0}, "↻", false, false, {0}, {0}, {0}, 0, -1 };
Button pauseBtn = { {0}, "❚❚", false, false, {0}, {0}, {0}, 0, -1 };
Button playBtn = { {0}, "▶ PLAY", false, false, {0}, {0}, {0}, 0, -1 };
Button resumeBtn = { {0}, "▶ RESUME", false, false, {0}, {0}, {0}, 0, -1 };
Button menuBtn = { {0}, "⌂ MENU", false, false, {0}, {0}, {0}, 0, -1 };

float sensSliderValue = 0.5f, volSliderValue = 0.7f;
float menuAnim = 0;

// Beautiful color scheme
Color COLOR_BG = { 20, 10, 30, 255 };
Color COLOR_PRIMARY = { 255, 80, 80, 255 };      // Red-orange
Color COLOR_SECONDARY = { 80, 200, 255, 255 };   // Cyan
Color COLOR_ACCENT = { 255, 200, 50, 255 };      // Gold
Color COLOR_DARK = { 30, 20, 40, 255 };
Color COLOR_GLOW = { 255, 100, 100, 100 };

int map[MAP_SIZE][MAP_SIZE] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,0,0,0,0,0,1,1,0,0,0,1},
    {1,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,1,1,1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,0,0,0,0,1,1,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

Vector2 spawnPoints[] = {
    {2.5f, 2.5f}, {13.5f, 2.5f}, {2.5f, 13.5f}, {13.5f, 13.5f},
    {5.5f, 5.5f}, {10.5f, 5.5f}, {5.5f, 10.5f}, {10.5f, 10.5f},
    {3.5f, 8.5f}, {12.5f, 8.5f}, {8.5f, 3.5f}, {8.5f, 12.5f}
};
int numSpawnPoints = 12;

Color demonColors[4] = { {255,80,80,255}, {255,150,50,255}, {180,80,255,255}, {150,50,50,255} };
const char* demonNames[4] = { "IMP", "DEMON", "CACO", "BARON" };

//------------------------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------------------------
float Distance(float x1, float y1, float x2, float y2) {
    return sqrtf((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
}

bool CheckWallCollision(float x, float y) {
    float offsets[5][2] = {{0,0}, {-0.25f,0}, {0.25f,0}, {0,-0.25f}, {0,0.25f}};
    for (int i = 0; i < 5; i++) {
        int mx = (int)(x + offsets[i][0]);
        int my = (int)(y + offsets[i][1]);
        if (mx < 0 || mx >= MAP_SIZE || my < 0 || my >= MAP_SIZE) return true;
        if (map[my][mx] == 1) return true;
    }
    return false;
}

bool IsValidSpawnPoint(float x, float y) {
    if (CheckWallCollision(x, y)) return false;
    if (Distance(x, y, player.x, player.y) < 6.0f) return false;
    for (int i = 0; i < demonCount; i++) {
        if (demons[i].alive && Distance(x, y, demons[i].x, demons[i].y) < 2.0f) return false;
    }
    return true;
}

void SpawnParticle(float x, float y, Color color) {
    if (particleCount >= MAX_PARTICLES) return;
    particles[particleCount].pos = (Vector2){x, y};
    particles[particleCount].vel = (Vector2){(rand()%80-40)/25.0f, (rand()%80-40)/25.0f};
    particles[particleCount].life = 1.0f;
    particles[particleCount].color = color;
    particleCount++;
}

void UpdateParticles(float dt) {
    for (int i = particleCount - 1; i >= 0; i--) {
        particles[i].pos.x += particles[i].vel.x * dt;
        particles[i].pos.y += particles[i].vel.y * dt;
        particles[i].life -= dt * 2.5f;
        if (particles[i].life <= 0) particles[i] = particles[--particleCount];
    }
}

void SpawnDemon(void) {
    if (demonCount >= MAX_DEMONS) return;
    
    Vector2 validSpawns[12];
    int validCount = 0;
    
    for (int i = 0; i < numSpawnPoints; i++) {
        if (IsValidSpawnPoint(spawnPoints[i].x, spawnPoints[i].y)) {
            validSpawns[validCount++] = spawnPoints[i];
        }
    }
    
    float x, y;
    if (validCount > 0) {
        int idx = rand() % validCount;
        x = validSpawns[idx].x;
        y = validSpawns[idx].y;
    } else {
        int attempts = 0;
        do {
            x = 2.0f + (rand() % 12);
            y = 2.0f + (rand() % 12);
            attempts++;
        } while (!IsValidSpawnPoint(x, y) && attempts < 100);
        if (attempts >= 100) return;
    }
    
    demons[demonCount].x = x;
    demons[demonCount].y = y;
    demons[demonCount].vx = 0;
    demons[demonCount].vy = 0;
    demons[demonCount].hp = 100;
    demons[demonCount].speed = 2.0f + (rand() % 100) / 100.0f;
    demons[demonCount].type = rand() % 4;
    demons[demonCount].alive = true;
    demons[demonCount].spawnX = x;
    demons[demonCount].spawnY = y;
    demonCount++;
    
    for (int p = 0; p < 8; p++) SpawnParticle(x, y, COLOR_SECONDARY);
}

void CastRay(float rayAngle, float* distance, int* hitWall) {
    float rad = rayAngle * DEG2RAD;
    float rayDirX = cosf(rad), rayDirY = sinf(rad);
    int mapX = (int)player.x, mapY = (int)player.y;
    float deltaDistX = fabsf(1.0f / rayDirX), deltaDistY = fabsf(1.0f / rayDirY);
    float sideDistX, sideDistY;
    int stepX, stepY;
    
    if (rayDirX < 0) { stepX = -1; sideDistX = (player.x - mapX) * deltaDistX; }
    else { stepX = 1; sideDistX = (mapX + 1.0f - player.x) * deltaDistX; }
    
    if (rayDirY < 0) { stepY = -1; sideDistY = (player.y - mapY) * deltaDistY; }
    else { stepY = 1; sideDistY = (mapY + 1.0f - player.y) * deltaDistY; }
    
    int hit = 0, side = 0;
    float perpWallDist = 0;
    
    for (int i = 0; i < 50; i++) {
        if (sideDistX < sideDistY) { sideDistX += deltaDistX; mapX += stepX; side = 0; }
        else { sideDistY += deltaDistY; mapY += stepY; side = 1; }
        
        if (mapX < 0 || mapX >= MAP_SIZE || mapY < 0 || mapY >= MAP_SIZE) {
            hit = 1; perpWallDist = 20.0f; break;
        }
        if (map[mapY][mapX] == 1) {
            hit = 1;
            if (side == 0) perpWallDist = (mapX - player.x + (1 - stepX) / 2.0f) / rayDirX;
            else perpWallDist = (mapY - player.y + (1 - stepY) / 2.0f) / rayDirY;
            break;
        }
    }
    
    *distance = fabsf(perpWallDist);
    *hitWall = hit;
}

//------------------------------------------------------------------------------------
// BEAUTIFUL UI Initialization
//------------------------------------------------------------------------------------
void InitUI(void) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    
    // BIG joystick on left side
    moveJoystick.pos = (Vector2){ 200, screenH - 200 };
    moveJoystick.stickPos = moveJoystick.pos;
    moveJoystick.radius = 180;  // Much bigger
    moveJoystick.deadZone = 0.12f;
    
    // BIG buttons on right
    int fireSize = 180;      // Bigger fire button
    int reloadSize = 120;    // Bigger reload
    
    fireBtn.rect = (Rectangle){ screenW - fireSize - 40, screenH - fireSize - 40, fireSize, fireSize };
    reloadBtn.rect = (Rectangle){ screenW - reloadSize - 40, screenH - fireSize - reloadSize - 80, reloadSize, reloadSize };
    pauseBtn.rect = (Rectangle){ screenW - 130, 30, 100, 100 };
    
    // Colors
    fireBtn.bgColor = (Color){ 255, 60, 60, 255 };
    fireBtn.textColor = WHITE;
    fireBtn.glowColor = (Color){ 255, 100, 100, 150 };
    
    reloadBtn.bgColor = (Color){ 60, 180, 255, 255 };
    reloadBtn.textColor = WHITE;
    reloadBtn.glowColor = (Color){ 100, 200, 255, 150 };
    
    pauseBtn.bgColor = (Color){ 80, 80, 100, 200 };
    pauseBtn.textColor = WHITE;
    pauseBtn.glowColor = (Color){ 120, 120, 150, 100 };
    
    // Menu buttons
    float btnWidth = 400, btnHeight = 100;
    playBtn.rect = (Rectangle){ (screenW - btnWidth)/2, screenH/2, btnWidth, btnHeight };
    playBtn.bgColor = (Color){ 50, 200, 100, 255 };
    playBtn.textColor = WHITE;
    playBtn.glowColor = (Color){ 100, 255, 150, 150 };
    
    resumeBtn.rect = (Rectangle){ (screenW - btnWidth)/2, screenH/2 - 60, btnWidth, btnHeight };
    resumeBtn.bgColor = (Color){ 50, 200, 100, 255 };
    resumeBtn.textColor = WHITE;
    resumeBtn.glowColor = (Color){ 100, 255, 150, 150 };
    
    menuBtn.rect = (Rectangle){ (screenW - btnWidth)/2, screenH/2 + 70, btnWidth, btnHeight };
    menuBtn.bgColor = (Color){ 100, 100, 120, 255 };
    menuBtn.textColor = WHITE;
    menuBtn.glowColor = (Color){ 150, 150, 180, 150 };
}

//------------------------------------------------------------------------------------
// Input Handling
//------------------------------------------------------------------------------------
void UpdateJoystick(Joystick* js, float* outX, float* outY) {
    *outX = 0; *outY = 0;
    int touchCount = GetTouchPointCount();
    
    if (js->active && js->touchId >= touchCount) {
        js->active = false;
        js->touchId = -1;
        js->stickPos = js->pos;
    }
    
    for (int i = 0; i < touchCount; i++) {
        Vector2 touch = GetTouchPosition(i);
        if (touch.x == 0 && touch.y == 0) continue;
        
        // Bigger activation area
        if (!js->active && touch.x < GetScreenWidth() * 0.45f) {
            float dist = Distance(touch.x, touch.y, js->pos.x, js->pos.y);
            if (dist < js->radius * 2.5f) {
                js->active = true;
                js->touchId = i;
            }
        }
        
        if (js->active && js->touchId == i) {
            Vector2 delta = { touch.x - js->pos.x, touch.y - js->pos.y };
            float dist = sqrtf(delta.x * delta.x + delta.y * delta.y);
            
            if (dist > js->radius) {
                delta.x = (delta.x / dist) * js->radius;
                delta.y = (delta.y / dist) * js->radius;
            }
            
            js->stickPos = (Vector2){ js->pos.x + delta.x, js->pos.y + delta.y };
            
            // Apply deadzone
            float inputDist = dist / js->radius;
            if (inputDist > js->deadZone) {
                *outX = delta.x / js->radius;
                *outY = -(delta.y / js->radius);
            }
            return;
        }
    }
    
    if (!js->active) js->stickPos = js->pos;
}

bool IsPointInRect(Vector2 p, Rectangle r) {
    return p.x >= r.x && p.x <= r.x + r.width && p.y >= r.y && p.y <= r.y + r.height;
}

void UpdateButton(Button* btn) {
    btn->wasPressed = btn->pressed;
    btn->pressed = false;
    int touchCount = GetTouchPointCount();
    
    if (btn->touchId >= 0 && btn->touchId < touchCount) {
        Vector2 touch = GetTouchPosition(btn->touchId);
        if (touch.x != 0 || touch.y != 0) {
            if (IsPointInRect(touch, btn->rect)) btn->pressed = true;
            else btn->touchId = -1;
        } else btn->touchId = -1;
    }
    
    if (!btn->pressed) {
        for (int i = 0; i < touchCount; i++) {
            Vector2 touch = GetTouchPosition(i);
            if (touch.x == 0 && touch.y == 0) continue;
            if (IsPointInRect(touch, btn->rect)) {
                btn->pressed = true;
                btn->touchId = i;
                break;
            }
        }
    }
    
    // Glow effect
    if (btn->pressed) btn->glowIntensity = 1.0f;
    else btn->glowIntensity *= 0.9f;
}

bool ButtonJustPressed(Button* btn) {
    return btn->pressed && !btn->wasPressed;
}

void UpdateCameraControl(float dt) {
    int touchCount = GetTouchPointCount();
    int screenW = GetScreenWidth();
    
    if (cameraActive) {
        bool valid = false;
        for (int i = 0; i < touchCount; i++) {
            if (i == cameraTouchId) {
                Vector2 touch = GetTouchPosition(i);
                if (touch.x != 0 || touch.y != 0) valid = true;
                break;
            }
        }
        if (!valid) { cameraActive = false; cameraTouchId = -1; }
    }
    
    for (int i = 0; i < touchCount; i++) {
        Vector2 touch = GetTouchPosition(i);
        if (touch.x == 0 && touch.y == 0) continue;
        
        if (moveJoystick.active && moveJoystick.touchId == i) continue;
        if (IsPointInRect(touch, fireBtn.rect)) continue;
        if (IsPointInRect(touch, reloadBtn.rect)) continue;
        if (IsPointInRect(touch, pauseBtn.rect)) continue;
        
        if (!cameraActive && touch.x > screenW * 0.4f) {
            cameraActive = true;
            cameraTouchId = i;
            cameraStartPos = touch;
            cameraStartAngle = player.angle;
        }
        
        if (cameraActive && cameraTouchId == i) {
            float deltaX = touch.x - cameraStartPos.x;
            player.angle = cameraStartAngle + deltaX * lookSensitivity * 0.4f;
        }
    }
}

void Shoot(void) {
    if (player.ammo <= 0 || GetTime() - lastShot < 0.18f) return;
    
    player.ammo--;
    lastShot = GetTime();
    
    for (int i = 0; i < 6; i++) {
        SpawnParticle(player.x + cosf(player.angle * DEG2RAD) * 0.5f, 
                     player.y + sinf(player.angle * DEG2RAD) * 0.5f, COLOR_ACCENT);
    }
    
    float rayX = player.x, rayY = player.y;
    for (float d = 0; d < 20.0f; d += 0.05f) {
        rayX = player.x + cosf(player.angle * DEG2RAD) * d;
        rayY = player.y + sinf(player.angle * DEG2RAD) * d;
        
        if (CheckWallCollision(rayX, rayY)) break;
        
        for (int i = 0; i < demonCount; i++) {
            if (!demons[i].alive) continue;
            if (Distance(rayX, rayY, demons[i].x, demons[i].y) < 0.5f) {
                demons[i].hp -= 50;
                for (int p = 0; p < 4; p++) SpawnParticle(demons[i].x, demons[i].y, COLOR_PRIMARY);
                if (demons[i].hp <= 0) {
                    demons[i].alive = false;
                    player.score++;
                    for (int p = 0; p < 12; p++) SpawnParticle(demons[i].x, demons[i].y, demonColors[demons[i].type]);
                }
                return;
            }
        }
    }
}

//------------------------------------------------------------------------------------
// Game Update
//------------------------------------------------------------------------------------
void UpdateGame(float dt) {
    if (gameState != PLAYING) return;
    
    UpdateButton(&fireBtn);
    UpdateButton(&reloadBtn);
    UpdateButton(&pauseBtn);
    
    if (ButtonJustPressed(&fireBtn)) Shoot();
    if (ButtonJustPressed(&reloadBtn) && player.ammo < 30) player.ammo = 30;
    if (ButtonJustPressed(&pauseBtn)) { gameState = PAUSED; return; }
    
    float joyX, joyY;
    UpdateJoystick(&moveJoystick, &joyX, &joyY);
    
    player.vx += joyX * ACCEL * dt;
    player.vy -= joyY * ACCEL * dt;
    
    if (IsKeyDown(KEY_W)) {
        player.vx += cosf(player.angle * DEG2RAD) * ACCEL * dt;
        player.vy += sinf(player.angle * DEG2RAD) * ACCEL * dt;
    }
    if (IsKeyDown(KEY_S)) {
        player.vx -= cosf(player.angle * DEG2RAD) * ACCEL * dt;
        player.vy -= sinf(player.angle * DEG2RAD) * ACCEL * dt;
    }
    if (IsKeyDown(KEY_A)) {
        player.vx += cosf((player.angle - 90) * DEG2RAD) * ACCEL * dt;
        player.vy += sinf((player.angle - 90) * DEG2RAD) * ACCEL * dt;
    }
    if (IsKeyDown(KEY_D)) {
        player.vx += cosf((player.angle + 90) * DEG2RAD) * ACCEL * dt;
        player.vy += sinf((player.angle + 90) * DEG2RAD) * ACCEL * dt;
    }
    
    player.vx *= (1.0f - FRICTION * dt);
    player.vy *= (1.0f - FRICTION * dt);
    
    float speed = sqrtf(player.vx * player.vx + player.vy * player.vy);
    if (speed > MAX_SPEED) {
        player.vx = (player.vx / speed) * MAX_SPEED;
        player.vy = (player.vy / speed) * MAX_SPEED;
    }
    
    float newX = player.x + player.vx * dt;
    float newY = player.y + player.vy * dt;
    
    if (!CheckWallCollision(newX, player.y)) player.x = newX;
    else player.vx = -player.vx * 0.3f;
    
    if (!CheckWallCollision(player.x, newY)) player.y = newY;
    else player.vy = -player.vy * 0.3f;
    
    UpdateCameraControl(dt);
    if (IsKeyDown(KEY_LEFT)) player.angle -= 100.0f * dt * lookSensitivity;
    if (IsKeyDown(KEY_RIGHT)) player.angle += 100.0f * dt * lookSensitivity;
    
    if (IsKeyPressed(KEY_SPACE)) Shoot();
    if (IsKeyPressed(KEY_R) && player.ammo < 30) player.ammo = 30;
    if (IsKeyPressed(KEY_ESCAPE)) gameState = PAUSED;
    
    // Demon physics
    for (int i = 0; i < demonCount; i++) {
        if (!demons[i].alive) continue;
        
        float dx = player.x - demons[i].x;
        float dy = player.y - demons[i].y;
        float dist = sqrtf(dx*dx + dy*dy);
        
        demons[i].distance = dist;
        demons[i].angle = atan2f(dy, dx) * RAD2DEG;
        
        if (dist > 0.8f) {
            float ax = (dx / dist) * DEMON_ACCEL;
            float ay = (dy / dist) * DEMON_ACCEL;
            
            demons[i].vx += ax * dt;
            demons[i].vy += ay * dt;
            
            demons[i].vx *= (1.0f - DEMON_FRICTION * dt);
            demons[i].vy *= (1.0f - DEMON_FRICTION * dt);
            
            float dSpeed = sqrtf(demons[i].vx * demons[i].vx + demons[i].vy * demons[i].vy);
            if (dSpeed > demons[i].speed) {
                demons[i].vx = (demons[i].vx / dSpeed) * demons[i].speed;
                demons[i].vy = (demons[i].vy / dSpeed) * demons[i].speed;
            }
            
            float newDX = demons[i].x + demons[i].vx * dt;
            float newDY = demons[i].y + demons[i].vy * dt;
            
            if (!CheckWallCollision(newDX, demons[i].y)) demons[i].x = newDX;
            else demons[i].vx = -demons[i].vx * 0.5f;
            
            if (!CheckWallCollision(demons[i].x, newDY)) demons[i].y = newDY;
            else demons[i].vy = -demons[i].vy * 0.5f;
        } else {
            player.health -= 1;
            if (player.health <= 0) gameState = GAME_OVER;
        }
    }
    
    if (GetTime() - lastSpawn > SPAWN_INTERVAL && demonCount < MAX_DEMONS) {
        SpawnDemon();
        lastSpawn = GetTime();
    }
    
    UpdateParticles(dt);
}

void ResetGame(void) {
    player.health = 100; player.ammo = 30; player.score = 0;
    player.x = 2.5f; player.y = 2.5f; player.vx = 0; player.vy = 0; player.angle = 0;
    demonCount = 0; particleCount = 0; lastSpawn = GetTime();
    gameState = PLAYING;
    SpawnDemon(); SpawnDemon();
}

//------------------------------------------------------------------------------------
// BEAUTIFUL Rendering
//------------------------------------------------------------------------------------
void DrawGame3D(void) {
    int screenW = GetScreenWidth(), screenH = GetScreenHeight();
    int numRays = screenW / 2;
    
    for (int i = 0; i < numRays; i++) {
        float rayAngle = player.angle - FOV/2 + (FOV * i / numRays);
        float distance; int hitWall;
        
        CastRay(rayAngle, &distance, &hitWall);
        
        float correctedDist = distance * cosf((rayAngle - player.angle) * DEG2RAD);
        float wallHeight = (screenH / correctedDist) * 0.8f;
        int wallTop = (screenH - wallHeight) / 2;
        
        float shade = fmaxf(0.15f, 1.0f - correctedDist/12.0f);
        unsigned char bright = (unsigned char)(255 * shade);
        Color wallColor = { (unsigned char)(bright*0.7f), (unsigned char)(bright*0.5f), (unsigned char)(bright*0.6f), 255 };
        
        DrawRectangle(i * 2, wallTop, 2, wallHeight, wallColor);
        DrawRectangle(i * 2, wallTop + wallHeight, 2, screenH - wallTop - wallHeight, (Color){ 20, 10, 25, 255 });
        DrawRectangle(i * 2, 0, 2, wallTop, (Color){ 35, 15, 30, 255 });
    }
    
    for (int i = 0; i < demonCount - 1; i++) {
        for (int j = i + 1; j < demonCount; j++) {
            if (demons[j].distance > demons[i].distance) {
                Demon temp = demons[i]; demons[i] = demons[j]; demons[j] = temp;
            }
        }
    }
    
    for (int i = 0; i < demonCount; i++) {
        if (!demons[i].alive) continue;
        
        float angleDiff = demons[i].angle - player.angle;
        while (angleDiff > 180) angleDiff -= 360;
        while (angleDiff < -180) angleDiff += 360;
        
        if (fabsf(angleDiff) < FOV/2 + 10) {
            float screenX = screenW/2 + (angleDiff / FOV) * screenW;
            float size = (screenH / demons[i].distance) * 0.5f;
            if (size > 300) size = 300;
            
            float shade = fmaxf(0.3f, 1.0f - demons[i].distance/10.0f);
            Color demonColor = demonColors[demons[i].type];
            demonColor.r = (unsigned char)(demonColor.r * shade);
            demonColor.g = (unsigned char)(demonColor.g * shade);
            demonColor.b = (unsigned char)(demonColor.b * shade);
            
            DrawRectangle(screenX - size/2, screenH/2 - size/2, size, size, demonColor);
            
            DrawRectangle(screenX - size/2, screenH/2 - size/2 - 25, size, 12, (Color){40,40,40,200});
            DrawRectangle(screenX - size/2, screenH/2 - size/2 - 25, size * (demons[i].hp/100.0f), 12, (Color){100,255,100,255});
            DrawText(demonNames[demons[i].type], screenX - 30, screenH/2 - size/2 - 50, 20, WHITE);
        }
    }
    
    for (int i = 0; i < particleCount; i++) {
        float screenX = screenW/2 + (particles[i].pos.x - player.x) * 40;
        float screenY = screenH/2 + (particles[i].pos.y - player.y) * 40;
        Color pColor = particles[i].color;
        pColor.a = (unsigned char)(255 * particles[i].life);
        DrawCircle(screenX, screenY, 4 * particles[i].life, pColor);
    }
}

void DrawBeautifulCrosshair(void) {
    int cx = GetScreenWidth() / 2;
    int cy = GetScreenHeight() / 2;
    
    // Outer glow
    DrawCircleLines(cx, cy, 28, (Color){255, 100, 100, 80});
    DrawCircleLines(cx, cy, 24, (Color){255, 150, 150, 150});
    
    // Cross lines with glow
    DrawLine(cx - 32, cy, cx + 32, cy, (Color){255, 200, 200, 200});
    DrawLine(cx, cy - 32, cx, cy + 32, (Color){255, 200, 200, 200});
    
    // Center dot
    DrawCircle(cx, cy, 4, COLOR_PRIMARY);
    DrawCircle(cx, cy, 2, WHITE);
}

void DrawBeautifulButton(Button* btn, int fontSize) {
    // Glow effect
    if (btn->glowIntensity > 0.01f) {
        Color glow = btn->glowColor;
        glow.a = (unsigned char)(btn->glowIntensity * 200);
        DrawRectangleRounded((Rectangle){ btn->rect.x - 8, btn->rect.y - 8, btn->rect.width + 16, btn->rect.height + 16 }, 0.3f, 16, glow);
    }
    
    // Button background
    Color bg = btn->pressed ? (Color){ btn->bgColor.r - 30, btn->bgColor.g - 30, btn->bgColor.b - 30, 255 } : btn->bgColor;
    DrawRectangleRounded(btn->rect, 0.25f, 16, bg);
    
    // Inner highlight
    DrawRectangleRounded((Rectangle){ btn->rect.x + 4, btn->rect.y + 4, btn->rect.width - 8, btn->rect.height/2 - 4 }, 0.2f, 8, (Color){255,255,255,30});
    
    // Border
    DrawRectangleRoundedLines(btn->rect, 0.25f, 16, (Color){255,255,255,150});
    
    // Text
    int textWidth = MeasureText(btn->label, fontSize);
    DrawText(btn->label, btn->rect.x + (btn->rect.width - textWidth)/2, 
             btn->rect.y + (btn->rect.height - fontSize)/2, fontSize, btn->textColor);
}

void DrawBeautifulJoystick(void) {
    Joystick* js = &moveJoystick;
    
    // Outer ring glow
    DrawCircleV(js->pos, js->radius + 15, (Color){100, 200, 255, 40});
    DrawCircleV(js->pos, js->radius + 8, (Color){100, 200, 255, 60});
    
    // Outer ring
    DrawCircleV(js->pos, js->radius, (Color){40, 40, 60, 180});
    DrawRing(js->pos, js->radius - 5, js->radius, 0, 360, 32, (Color){100, 200, 255, 100});
    
    // Direction markers
    DrawLineV((Vector2){js->pos.x, js->pos.y - js->radius + 20}, (Vector2){js->pos.x, js->pos.y - js->radius + 40}, (Color){255,255,255,80});
    DrawLineV((Vector2){js->pos.x, js->pos.y + js->radius - 20}, (Vector2){js->pos.x, js->pos.y + js->radius - 40}, (Color){255,255,255,80});
    DrawLineV((Vector2){js->pos.x - js->radius + 20, js->pos.y}, (Vector2){js->pos.x - js->radius + 40, js->pos.y}, (Color){255,255,255,80});
    DrawLineV((Vector2){js->pos.x + js->radius - 20, js->pos.y}, (Vector2){js->pos.x + js->radius - 40, js->pos.y}, (Color){255,255,255,80});
    
    // Stick
    float stickDist = Distance(js->stickPos.x, js->stickPos.y, js->pos.x, js->pos.y);
    if (stickDist > 5) {
        // Stick glow when active
        DrawCircleV(js->stickPos, js->radius * 0.4f + 10, (Color){100, 200, 255, 100});
    }
    
    DrawCircleV(js->stickPos, js->radius * 0.4f, (Color){80, 180, 255, 255});
    DrawCircleV(js->stickPos, js->radius * 0.25f, (Color){150, 220, 255, 255});
    DrawCircleV((Vector2){js->stickPos.x - 8, js->stickPos.y - 8}, js->radius * 0.1f, (Color){255,255,255,150});
}

void DrawGameUI(void) {
    // Beautiful joystick
    DrawBeautifulJoystick();
    
    // Beautiful buttons
    DrawBeautifulButton(&fireBtn, 48);
    DrawBeautifulButton(&reloadBtn, 42);
    DrawBeautifulButton(&pauseBtn, 32);
    
    // HUD with style
    DrawText(TextFormat("♥ %d", player.health), 30, 30, 48, COLOR_PRIMARY);
    DrawText(TextFormat("⌖ %d", player.ammo), GetScreenWidth() - 160, 30, 48, COLOR_SECONDARY);
    DrawText(TextFormat("KILLS: %d", player.score), 30, 90, 32, COLOR_ACCENT);
    
    DrawBeautifulCrosshair();
}

void DrawBeautifulMenu(void) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    
    // Animated background
    menuAnim += 0.02f;
    
    // Gradient background
    DrawRectangle(0, 0, w, h, COLOR_BG);
    
    // Decorative circles
    for (int i = 0; i < 5; i++) {
        float offset = sinf(menuAnim + i) * 20;
        DrawCircle(w/2 + offset + i * 50, h/3 + offset, 100 + i * 30, (Color){40 + i*10, 20, 50 + i*10, 100});
    }
    
    // Title with glow
    DrawText("DEMON FPS", w/2 - 220, h/3 - 60, 90, (Color){100, 20, 20, 255});
    DrawText("DEMON FPS", w/2 - 225, h/3 - 65, 90, COLOR_PRIMARY);
    
    DrawText("HELL AWAITS", w/2 - 140, h/3 + 50, 32, (Color){150, 150, 150, 255});
    
    UpdateButton(&playBtn);
    DrawBeautifulButton(&playBtn, 42);
    
    if (ButtonJustPressed(&playBtn)) ResetGame();
}

void DrawPauseMenu(void) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    
    DrawRectangle(0, 0, w, h, (Color){ 0, 0, 0, 200 });
    
    DrawText("PAUSED", w/2 - 130, h/2 - 200, 70, WHITE);
    
    // Sensitivity slider
    DrawText("LOOK SENSITIVITY", w/2 - 140, h/2 - 100, 26, WHITE);
    Rectangle sensRect = { w/2 - 200, h/2 - 55, 400, 30 };
    DrawRectangleRec(sensRect, (Color){50,50,60,255});
    DrawRectangleRec((Rectangle){ sensRect.x, sensRect.y, sensRect.width * sensSliderValue, sensRect.height }, COLOR_SECONDARY);
    DrawCircle(sensRect.x + sensRect.width * sensSliderValue, sensRect.y + 15, 22, WHITE);
    DrawCircle(sensRect.x + sensRect.width * sensSliderValue, sensRect.y + 15, 18, COLOR_SECONDARY);
    
    // Volume slider
    DrawText("VOLUME", w/2 - 70, h/2 + 10, 26, WHITE);
    Rectangle volRect = { w/2 - 200, h/2 + 50, 400, 30 };
    DrawRectangleRec(volRect, (Color){50,50,60,255});
    DrawRectangleRec((Rectangle){ volRect.x, volRect.y, volRect.width * volSliderValue, volRect.height }, (Color){100,255,100,255});
    DrawCircle(volRect.x + volRect.width * volSliderValue, volRect.y + 15, 22, WHITE);
    DrawCircle(volRect.x + volRect.width * volSliderValue, volRect.y + 15, 18, (Color){100,255,100,255});
    
    // Handle sliders
    int touchCount = GetTouchPointCount();
    for (int i = 0; i < touchCount; i++) {
        Vector2 touch = GetTouchPosition(i);
        if (touch.x == 0 && touch.y == 0) continue;
        
        if (touch.y >= sensRect.y - 20 && touch.y <= sensRect.y + sensRect.height + 20) {
            if (touch.x >= sensRect.x && touch.x <= sensRect.x + sensRect.width) {
                sensSliderValue = (touch.x - sensRect.x) / sensRect.width;
                if (sensSliderValue < 0) sensSliderValue = 0;
                if (sensSliderValue > 1) sensSliderValue = 1;
                lookSensitivity = 0.4f + sensSliderValue * 2.0f;
            }
        }
        
        if (touch.y >= volRect.y - 20 && touch.y <= volRect.y + volRect.height + 20) {
            if (touch.x >= volRect.x && touch.x <= volRect.x + volRect.width) {
                volSliderValue = (touch.x - volRect.x) / volRect.width;
                if (volSliderValue < 0) volSliderValue = 0;
                if (volSliderValue > 1) volSliderValue = 1;
                masterVolume = volSliderValue;
            }
        }
    }
    
    UpdateButton(&resumeBtn);
    UpdateButton(&menuBtn);
    DrawBeautifulButton(&resumeBtn, 38);
    DrawBeautifulButton(&menuBtn, 38);
    
    if (ButtonJustPressed(&resumeBtn)) gameState = PLAYING;
    if (ButtonJustPressed(&menuBtn)) gameState = MENU;
}

void DrawGameOver(void) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    
    DrawRectangle(0, 0, w, h, (Color){ 0, 0, 0, 220 });
    DrawText("GAME OVER", w/2 - 220, h/2 - 100, 80, COLOR_PRIMARY);
    DrawText(TextFormat("Demons Killed: %d", player.score), w/2 - 180, h/2 + 10, 38, WHITE);
    DrawText("Tap to continue", w/2 - 140, h/2 + 90, 30, (Color){180,180,180,255});
    
    if (GetTouchPointCount() > 0) {
        Vector2 touch = GetTouchPosition(0);
        if (touch.x != 0 || touch.y != 0) gameState = MENU;
    }
}

//------------------------------------------------------------------------------------
// Main Entry Point
//------------------------------------------------------------------------------------
int main(void) {
    InitWindow(0, 0, "Demon FPS");
    SetTargetFPS(60);
    
    srand(time(NULL));
    InitUI();
    
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        UpdateGame(dt);
        
        BeginDrawing();
        ClearBackground(BLACK);
        
        switch (gameState) {
            case MENU:
                DrawBeautifulMenu();
                break;
            case PLAYING:
                DrawGame3D();
                DrawGameUI();
                break;
            case PAUSED:
                DrawGame3D();
                DrawGameUI();
                DrawPauseMenu();
                break;
            case GAME_OVER:
                DrawGame3D();
                DrawGameOver();
                break;
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}
