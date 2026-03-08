/*******************************************************************************************
*
*   Demon FPS - Fully Fixed Version
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
#define MOVE_SPEED 3.0f
#define ROT_SPEED 2.0f
#define MAX_DEMONS 10
#define MAX_PARTICLES 30

typedef enum { MENU, PLAYING, PAUSED, GAME_OVER } GameState;

//------------------------------------------------------------------------------------
// Types and Structures
//------------------------------------------------------------------------------------
typedef struct {
    float x, y;
    float angle;
    int health;
    int ammo;
    int score;
} Player;

typedef struct {
    float x, y;
    float hp;
    float speed;
    int type;
    bool alive;
    float distance;
    float angle;
} Demon;

typedef struct {
    Vector2 pos;
    float radius;
    Vector2 stickPos;
    bool active;
    int touchId;
} Joystick;

typedef struct {
    Rectangle rect;
    const char* label;
    bool pressed;
    bool wasPressed;
    Color color;
    Color pressColor;
    int touchId;
} Button;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float life;
    Color color;
} Particle;

//------------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------------
Player player = { 2.5f, 2.5f, 0.0f, 100, 30, 0 };
Demon demons[MAX_DEMONS];
Particle particles[MAX_PARTICLES];
int demonCount = 0;
int particleCount = 0;
float lastShot = 0;
GameState gameState = MENU;
float lookSensitivity = 1.0f;
float masterVolume = 0.7f;

// Camera control
bool cameraActive = false;
int cameraTouchId = -1;
Vector2 cameraStartPos = {0};
float cameraStartAngle = 0;

// UI Elements
Joystick moveJoystick = { {0}, 140, {0}, false, -1 };
Button fireBtn = { {0}, "FIRE", false, false, RED, (Color){180, 0, 0, 255}, -1 };
Button reloadBtn = { {0}, "R", false, false, ORANGE, (Color){200, 130, 0, 255}, -1 };
Button pauseBtn = { {0}, "||", false, false, GRAY, (Color){80, 80, 80, 255}, -1 };
Button playBtn = { {0}, "PLAY", false, false, GREEN, (Color){0, 150, 0, 255}, -1 };
Button resumeBtn = { {0}, "RESUME", false, false, GREEN, (Color){0, 150, 0, 255}, -1 };
Button menuBtn = { {0}, "MENU", false, false, GRAY, (Color){100, 100, 100, 255}, -1 };

float sensSliderValue = 0.5f;
float volSliderValue = 0.7f;

// FIXED MAP - Proper walls
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

Color demonColors[4] = { RED, ORANGE, PURPLE, MAROON };
const char* demonNames[4] = { "IMP", "DEMON", "CACO", "BARON" };

//------------------------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------------------------
float Distance(float x1, float y1, float x2, float y2) {
    return sqrtf((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
}

bool CheckWallCollision(float x, float y) {
    int mapX = (int)x;
    int mapY = (int)y;
    if (mapX < 0 || mapX >= MAP_SIZE || mapY < 0 || mapY >= MAP_SIZE) return true;
    return map[mapY][mapX] == 1;
}

void SpawnParticle(float x, float y, Color color) {
    if (particleCount >= MAX_PARTICLES) return;
    particles[particleCount].pos = (Vector2){x, y};
    particles[particleCount].vel = (Vector2){(rand()%60-30)/20.0f, (rand()%60-30)/20.0f};
    particles[particleCount].life = 1.0f;
    particles[particleCount].color = color;
    particleCount++;
}

void UpdateParticles(float dt) {
    for (int i = particleCount - 1; i >= 0; i--) {
        particles[i].pos.x += particles[i].vel.x * dt;
        particles[i].pos.y += particles[i].vel.y * dt;
        particles[i].life -= dt * 2.5f;
        if (particles[i].life <= 0) {
            particles[i] = particles[--particleCount];
        }
    }
}

void SpawnDemon(void) {
    if (demonCount >= MAX_DEMONS) return;
    
    float x, y;
    int attempts = 0;
    do {
        x = 2.0f + (rand() % 12);
        y = 2.0f + (rand() % 12);
        attempts++;
    } while ((CheckWallCollision(x, y) || Distance(x, y, player.x, player.y) < 4.0f) && attempts < 100);
    
    if (attempts < 100) {
        demons[demonCount].x = x + 0.5f;
        demons[demonCount].y = y + 0.5f;
        demons[demonCount].hp = 100;
        demons[demonCount].speed = 0.8f + (rand() % 60) / 100.0f;
        demons[demonCount].type = rand() % 4;
        demons[demonCount].alive = true;
        demonCount++;
    }
}

void CastRay(float rayAngle, float* distance, int* hitWall) {
    float rad = rayAngle * DEG2RAD;
    float rayDirX = cosf(rad);
    float rayDirY = sinf(rad);
    
    int mapX = (int)player.x;
    int mapY = (int)player.y;
    
    float deltaDistX = fabsf(1.0f / rayDirX);
    float deltaDistY = fabsf(1.0f / rayDirY);
    
    float sideDistX, sideDistY;
    int stepX, stepY;
    
    if (rayDirX < 0) {
        stepX = -1;
        sideDistX = (player.x - mapX) * deltaDistX;
    } else {
        stepX = 1;
        sideDistX = (mapX + 1.0f - player.x) * deltaDistX;
    }
    
    if (rayDirY < 0) {
        stepY = -1;
        sideDistY = (player.y - mapY) * deltaDistY;
    } else {
        stepY = 1;
        sideDistY = (mapY + 1.0f - player.y) * deltaDistY;
    }
    
    int hit = 0;
    int side = 0;
    float perpWallDist = 0;
    
    for (int i = 0; i < 50; i++) {
        if (sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
        } else {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
        }
        
        if (mapX < 0 || mapX >= MAP_SIZE || mapY < 0 || mapY >= MAP_SIZE) {
            hit = 1;
            perpWallDist = 20.0f;
            break;
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
// UI Initialization
//------------------------------------------------------------------------------------
void InitUI(void) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    
    moveJoystick.pos = (Vector2){ 160, screenH - 160 };
    moveJoystick.stickPos = moveJoystick.pos;
    moveJoystick.radius = 140;
    
    int btnSize = 130;
    int smallBtnSize = 90;
    
    fireBtn.rect = (Rectangle){ screenW - btnSize - 30, screenH - btnSize - 30, btnSize, btnSize };
    reloadBtn.rect = (Rectangle){ screenW - smallBtnSize - 30, screenH - btnSize - smallBtnSize - 60, smallBtnSize, smallBtnSize };
    pauseBtn.rect = (Rectangle){ screenW - 110, 20, 90, 90 };
    
    float btnWidth = 350, btnHeight = 90;
    playBtn.rect = (Rectangle){ (screenW - btnWidth)/2, screenH/2, btnWidth, btnHeight };
    resumeBtn.rect = (Rectangle){ (screenW - btnWidth)/2, screenH/2 - 60, btnWidth, btnHeight };
    menuBtn.rect = (Rectangle){ (screenW - btnWidth)/2, screenH/2 + 50, btnWidth, btnHeight };
}

//------------------------------------------------------------------------------------
// Input Handling
//------------------------------------------------------------------------------------
void UpdateJoystick(Joystick* js, float* outX, float* outY) {
    *outX = 0; *outY = 0;
    int touchCount = GetTouchPointCount();
    
    // Check if active touch is still valid
    if (js->active && js->touchId >= touchCount) {
        js->active = false;
        js->touchId = -1;
        js->stickPos = js->pos;
    }
    
    for (int i = 0; i < touchCount; i++) {
        Vector2 touch = GetTouchPosition(i);
        if (touch.x == 0 && touch.y == 0) continue;
        
        // Start joystick on left side
        if (!js->active && touch.x < GetScreenWidth() * 0.4f) {
            float dist = Distance(touch.x, touch.y, js->pos.x, js->pos.y);
            if (dist < js->radius * 2.0f) {
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
            *outX = delta.x / js->radius;
            *outY = -(delta.y / js->radius);
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
    
    // Check existing touch
    if (btn->touchId >= 0 && btn->touchId < touchCount) {
        Vector2 touch = GetTouchPosition(btn->touchId);
        if (touch.x != 0 || touch.y != 0) {
            if (IsPointInRect(touch, btn->rect)) {
                btn->pressed = true;
            } else {
                btn->touchId = -1;
            }
        } else {
            btn->touchId = -1;
        }
    }
    
    // Find new touch
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
}

bool ButtonJustPressed(Button* btn) {
    return btn->pressed && !btn->wasPressed;
}

void UpdateCameraControl(float dt) {
    int touchCount = GetTouchPointCount();
    int screenW = GetScreenWidth();
    
    // Check if camera touch still valid
    if (cameraActive) {
        bool valid = false;
        for (int i = 0; i < touchCount; i++) {
            if (i == cameraTouchId) {
                Vector2 touch = GetTouchPosition(i);
                if (touch.x != 0 || touch.y != 0) valid = true;
                break;
            }
        }
        if (!valid) {
            cameraActive = false;
            cameraTouchId = -1;
        }
    }
    
    for (int i = 0; i < touchCount; i++) {
        Vector2 touch = GetTouchPosition(i);
        if (touch.x == 0 && touch.y == 0) continue;
        
        // Skip joystick touch
        if (moveJoystick.active && moveJoystick.touchId == i) continue;
        
        // Skip buttons
        if (IsPointInRect(touch, fireBtn.rect)) continue;
        if (IsPointInRect(touch, reloadBtn.rect)) continue;
        if (IsPointInRect(touch, pauseBtn.rect)) continue;
        
        // Right side for camera
        if (!cameraActive && touch.x > screenW * 0.35f) {
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
    if (player.ammo <= 0 || GetTime() - lastShot < 0.2f) return;
    
    player.ammo--;
    lastShot = GetTime();
    
    for (int i = 0; i < 5; i++) {
        SpawnParticle(player.x + cosf(player.angle * DEG2RAD) * 0.5f, 
                     player.y + sinf(player.angle * DEG2RAD) * 0.5f, YELLOW);
    }
    
    float rayX = player.x;
    float rayY = player.y;
    for (float d = 0; d < 20.0f; d += 0.05f) {
        rayX = player.x + cosf(player.angle * DEG2RAD) * d;
        rayY = player.y + sinf(player.angle * DEG2RAD) * d;
        
        if (CheckWallCollision(rayX, rayY)) break;
        
        for (int i = 0; i < demonCount; i++) {
            if (!demons[i].alive) continue;
            if (Distance(rayX, rayY, demons[i].x, demons[i].y) < 0.5f) {
                demons[i].hp -= 50;
                for (int p = 0; p < 3; p++) SpawnParticle(demons[i].x, demons[i].y, RED);
                if (demons[i].hp <= 0) {
                    demons[i].alive = false;
                    player.score++;
                    for (int p = 0; p < 8; p++) SpawnParticle(demons[i].x, demons[i].y, demonColors[demons[i].type]);
                }
                return;
            }
        }
    }
}

//------------------------------------------------------------------------------------
// Game Update - FIXED MOVEMENT AND AI
//------------------------------------------------------------------------------------
void UpdateGame(float dt) {
    if (gameState != PLAYING) return;
    
    // Update buttons
    UpdateButton(&fireBtn);
    UpdateButton(&reloadBtn);
    UpdateButton(&pauseBtn);
    
    if (ButtonJustPressed(&fireBtn)) Shoot();
    if (ButtonJustPressed(&reloadBtn) && player.ammo < 30) player.ammo = 30;
    if (ButtonJustPressed(&pauseBtn)) { gameState = PAUSED; return; }
    
    // Movement joystick - SMOOTH
    float joyX, joyY;
    UpdateJoystick(&moveJoystick, &joyX, &joyY);
    
    // Apply movement with collision - CHECK BOTH AXES SEPARATELY
    float moveStep = MOVE_SPEED * dt;
    
    // X movement
    float newX = player.x + joyX * moveStep;
    if (!CheckWallCollision(newX + (joyX > 0 ? 0.2f : -0.2f), player.y)) {
        player.x = newX;
    }
    
    // Y movement  
    float newY = player.y - joyY * moveStep;
    if (!CheckWallCollision(player.x, newY + (-joyY > 0 ? 0.2f : -0.2f))) {
        player.y = newY;
    }
    
    // Keyboard fallback
    if (IsKeyDown(KEY_W)) {
        float nx = player.x + cosf(player.angle * DEG2RAD) * moveStep;
        float ny = player.y + sinf(player.angle * DEG2RAD) * moveStep;
        if (!CheckWallCollision(nx, player.y)) player.x = nx;
        if (!CheckWallCollision(player.x, ny)) player.y = ny;
    }
    if (IsKeyDown(KEY_S)) {
        float nx = player.x - cosf(player.angle * DEG2RAD) * moveStep;
        float ny = player.y - sinf(player.angle * DEG2RAD) * moveStep;
        if (!CheckWallCollision(nx, player.y)) player.x = nx;
        if (!CheckWallCollision(player.x, ny)) player.y = ny;
    }
    
    // Camera
    UpdateCameraControl(dt);
    if (IsKeyDown(KEY_LEFT)) player.angle -= 100.0f * dt * lookSensitivity;
    if (IsKeyDown(KEY_RIGHT)) player.angle += 100.0f * dt * lookSensitivity;
    
    // Keyboard shoot/reload/pause
    if (IsKeyPressed(KEY_SPACE)) Shoot();
    if (IsKeyPressed(KEY_R) && player.ammo < 30) player.ammo = 30;
    if (IsKeyPressed(KEY_ESCAPE)) gameState = PAUSED;
    
    // FIXED DEMON AI - Better collision and movement
    for (int i = 0; i < demonCount; i++) {
        if (!demons[i].alive) continue;
        
        float dx = player.x - demons[i].x;
        float dy = player.y - demons[i].y;
        float dist = sqrtf(dx*dx + dy*dy);
        
        demons[i].distance = dist;
        demons[i].angle = atan2f(dy, dx) * RAD2DEG;
        
        if (dist > 0.8f) {
            // Move toward player with collision
            float moveX = (dx / dist) * demons[i].speed * dt;
            float moveY = (dy / dist) * demons[i].speed * dt;
            
            float newDemonX = demons[i].x + moveX;
            float newDemonY = demons[i].y + moveY;
            
            // Check collision before moving
            if (!CheckWallCollision(newDemonX, demons[i].y)) {
                demons[i].x = newDemonX;
            }
            if (!CheckWallCollision(demons[i].x, newDemonY)) {
                demons[i].y = newDemonY;
            }
        } else {
            // Attack player
            player.health -= 1;
            if (player.health <= 0) gameState = GAME_OVER;
        }
    }
    
    // Spawn demons
    if (GetTime() > (float)(player.score * 2 + 5) && demonCount < MAX_DEMONS) {
        SpawnDemon();
    }
    
    UpdateParticles(dt);
}

void ResetGame(void) {
    player.health = 100;
    player.ammo = 30;
    player.score = 0;
    player.x = 2.5f;
    player.y = 2.5f;
    player.angle = 0;
    demonCount = 0;
    particleCount = 0;
    gameState = PLAYING;
    for (int i = 0; i < 3; i++) SpawnDemon();
}

//------------------------------------------------------------------------------------
// Rendering
//------------------------------------------------------------------------------------
void DrawGame3D(void) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int numRays = screenW / 2;
    
    // Raycasting with proper DDA
    for (int i = 0; i < numRays; i++) {
        float rayAngle = player.angle - FOV/2 + (FOV * i / numRays);
        float distance;
        int hitWall;
        
        CastRay(rayAngle, &distance, &hitWall);
        
        // Fix fisheye
        float correctedDist = distance * cosf((rayAngle - player.angle) * DEG2RAD);
        float wallHeight = (screenH / correctedDist) * 0.8f;
        int wallTop = (screenH - wallHeight) / 2;
        
        // Better shading based on distance
        float shade = fmaxf(0.15f, 1.0f - correctedDist/12.0f);
        unsigned char bright = (unsigned char)(255 * shade);
        Color wallColor = { (unsigned char)(bright*0.6f), (unsigned char)(bright*0.4f), (unsigned char)(bright*0.4f), 255 };
        
        DrawRectangle(i * 2, wallTop, 2, wallHeight, wallColor);
        DrawRectangle(i * 2, wallTop + wallHeight, 2, screenH - wallTop - wallHeight, (Color){ 25, 12, 12, 255 });
        DrawRectangle(i * 2, 0, 2, wallTop, (Color){ 40, 8, 8, 255 });
    }
    
    // Sort demons by distance (painters algorithm)
    for (int i = 0; i < demonCount - 1; i++) {
        for (int j = i + 1; j < demonCount; j++) {
            if (demons[j].distance > demons[i].distance) {
                Demon temp = demons[i];
                demons[i] = demons[j];
                demons[j] = temp;
            }
        }
    }
    
    // Draw demons
    for (int i = 0; i < demonCount; i++) {
        if (!demons[i].alive) continue;
        
        float angleDiff = demons[i].angle - player.angle;
        while (angleDiff > 180) angleDiff -= 360;
        while (angleDiff < -180) angleDiff += 360;
        
        if (fabsf(angleDiff) < FOV/2 + 10) {
            float screenX = screenW/2 + (angleDiff / FOV) * screenW;
            float size = (screenH / demons[i].distance) * 0.5f;
            if (size > 300) size = 300; // Cap size
            
            float shade = fmaxf(0.3f, 1.0f - demons[i].distance/10.0f);
            Color demonColor = demonColors[demons[i].type];
            demonColor.r = (unsigned char)(demonColor.r * shade);
            demonColor.g = (unsigned char)(demonColor.g * shade);
            demonColor.b = (unsigned char)(demonColor.b * shade);
            
            DrawRectangle(screenX - size/2, screenH/2 - size/2, size, size, demonColor);
            
            // Health bar
            DrawRectangle(screenX - size/2, screenH/2 - size/2 - 25, size, 12, DARKGRAY);
            DrawRectangle(screenX - size/2, screenH/2 - size/2 - 25, size * (demons[i].hp/100.0f), 12, GREEN);
            DrawText(demonNames[demons[i].type], screenX - 30, screenH/2 - size/2 - 50, 20, WHITE);
        }
    }
    
    // Particles
    for (int i = 0; i < particleCount; i++) {
        float screenX = screenW/2 + (particles[i].pos.x - player.x) * 40;
        float screenY = screenH/2 + (particles[i].pos.y - player.y) * 40;
        Color pColor = particles[i].color;
        pColor.a = (unsigned char)(255 * particles[i].life);
        DrawCircle(screenX, screenY, 4 * particles[i].life, pColor);
    }
}

void DrawCrosshair(void) {
    int cx = GetScreenWidth() / 2;
    int cy = GetScreenHeight() / 2;
    
    DrawCircleLines(cx, cy, 20, GREEN);
    DrawLine(cx - 28, cy, cx + 28, cy, GREEN);
    DrawLine(cx, cy - 28, cx, cy + 28, GREEN);
    DrawCircle(cx, cy, 3, RED);
}

void DrawButton(Button* btn, int fontSize) {
    Color c = btn->pressed ? btn->pressColor : btn->color;
    DrawRectangleRounded(btn->rect, 0.2f, 8, c);
    DrawRectangleRoundedLines(btn->rect, 0.2f, 8, (Color){255,255,255,200});
    
    int textWidth = MeasureText(btn->label, fontSize);
    DrawText(btn->label, 
             btn->rect.x + (btn->rect.width - textWidth)/2, 
             btn->rect.y + (btn->rect.height - fontSize)/2, 
             fontSize, WHITE);
}

void DrawGameUI(void) {
    // Joystick
    DrawCircleV(moveJoystick.pos, moveJoystick.radius, (Color){ 255, 255, 255, 50 });
    DrawCircleV(moveJoystick.stickPos, moveJoystick.radius * 0.35f, (Color){ 255, 255, 255, 120 });
    
    // Buttons
    DrawButton(&fireBtn, 32);
    DrawButton(&reloadBtn, 36);
    DrawButton(&pauseBtn, 36);
    
    // HUD
    DrawText(TextFormat("♥ %d", player.health), 25, 25, 40, RED);
    DrawText(TextFormat("⌖ %d", player.ammo), GetScreenWidth() - 140, 25, 40, YELLOW);
    DrawText(TextFormat("KILLS: %d", player.score), 25, 75, 28, SKYBLUE);
    
    DrawCrosshair();
}

void DrawMenu(void) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    
    DrawRectangle(0, 0, w, h, (Color){ 20, 0, 0, 255 });
    
    DrawText("DEMON FPS", w/2 - 200, h/3 - 50, 80, RED);
    DrawText("HELL AWAITS", w/2 - 140, h/3 + 50, 30, DARKGRAY);
    
    UpdateButton(&playBtn);
    DrawButton(&playBtn, 36);
    
    if (ButtonJustPressed(&playBtn)) ResetGame();
}

void DrawPauseMenu(void) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    
    DrawRectangle(0, 0, w, h, (Color){ 0, 0, 0, 180 });
    
    DrawText("PAUSED", w/2 - 120, h/2 - 180, 60, WHITE);
    
    // Sensitivity
    DrawText("SENSITIVITY", w/2 - 120, h/2 - 80, 24, WHITE);
    Rectangle sensRect = { w/2 - 200, h/2 - 40, 400, 25 };
    DrawRectangleRec(sensRect, DARKGRAY);
    DrawRectangleRec((Rectangle){ sensRect.x, sensRect.y, sensRect.width * sensSliderValue, sensRect.height }, BLUE);
    DrawCircle(sensRect.x + sensRect.width * sensSliderValue, sensRect.y + 12, 18, WHITE);
    
    // Volume
    DrawText("VOLUME", w/2 - 70, h/2 + 10, 24, WHITE);
    Rectangle volRect = { w/2 - 200, h/2 + 45, 400, 25 };
    DrawRectangleRec(volRect, DARKGRAY);
    DrawRectangleRec((Rectangle){ volRect.x, volRect.y, volRect.width * volSliderValue, volRect.height }, GREEN);
    DrawCircle(volRect.x + volRect.width * volSliderValue, volRect.y + 12, 18, WHITE);
    
    // Handle sliders
    int touchCount = GetTouchPointCount();
    for (int i = 0; i < touchCount; i++) {
        Vector2 touch = GetTouchPosition(i);
        if (touch.x == 0 && touch.y == 0) continue;
        
        if (touch.y >= sensRect.y - 15 && touch.y <= sensRect.y + sensRect.height + 15) {
            if (touch.x >= sensRect.x && touch.x <= sensRect.x + sensRect.width) {
                sensSliderValue = (touch.x - sensRect.x) / sensRect.width;
                if (sensSliderValue < 0) sensSliderValue = 0;
                if (sensSliderValue > 1) sensSliderValue = 1;
                lookSensitivity = 0.4f + sensSliderValue * 2.0f;
            }
        }
        
        if (touch.y >= volRect.y - 15 && touch.y <= volRect.y + volRect.height + 15) {
            if (touch.x >= volRect.x && touch.x <= volRect.x + volRect.width) {
                volSliderValue = (touch.x - volRect.x) / volRect.width;
                if (volSliderValue < 0) volSliderValue = 0;
                if (volSliderValue > 1) volSliderValue = 1;
                masterVolume = volSliderValue;
            }
        }
    }
    
    // Buttons
    UpdateButton(&resumeBtn);
    UpdateButton(&menuBtn);
    DrawButton(&resumeBtn, 32);
    DrawButton(&menuBtn, 32);
    
    if (ButtonJustPressed(&resumeBtn)) gameState = PLAYING;
    if (ButtonJustPressed(&menuBtn)) gameState = MENU;
}

void DrawGameOver(void) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    
    DrawRectangle(0, 0, w, h, (Color){ 0, 0, 0, 200 });
    DrawText("GAME OVER", w/2 - 200, h/2 - 80, 70, RED);
    DrawText(TextFormat("Demons Killed: %d", player.score), w/2 - 170, h/2 + 10, 35, WHITE);
    DrawText("Tap to continue", w/2 - 130, h/2 + 80, 28, GRAY);
    
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
                DrawMenu();
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
