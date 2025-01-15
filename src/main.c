#include "raylib.h"
#include <stdint.h>

#define WIDTH 64
#define HEIGHT 32

uint8_t display[HEIGHT][WIDTH] = {0}; // 64x32

int GetChip8Key() {
    if (IsKeyPressed(KEY_ONE)) return 0x1;
    if (IsKeyPressed(KEY_TWO)) return 0x2;
    if (IsKeyPressed(KEY_THREE)) return 0x3;
    if (IsKeyPressed(KEY_FOUR)) return 0xC;
    if (IsKeyPressed(KEY_Q)) return 0x4;
    if (IsKeyPressed(KEY_W)) return 0x5;
    if (IsKeyPressed(KEY_E)) return 0x6;
    if (IsKeyPressed(KEY_R)) return 0xD;
    if (IsKeyPressed(KEY_A)) return 0x7;
    if (IsKeyPressed(KEY_S)) return 0x8;
    if (IsKeyPressed(KEY_D)) return 0x9;
    if (IsKeyPressed(KEY_F)) return 0xE;
    if (IsKeyPressed(KEY_Z)) return 0xA;
    if (IsKeyPressed(KEY_X)) return 0x0;
    if (IsKeyPressed(KEY_C)) return 0xB;
    if (IsKeyPressed(KEY_V)) return 0xF;
    return -1; 
}

void RenderDisplay(uint8_t d[][WIDTH]) {
    const int scale = 10;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (d[y][x] == 1) {
                DrawRectangle(x * scale, y * scale, scale, scale, WHITE);
            }
        }
    }
}

int main() {
    const int screenWidth = WIDTH * 10;
    const int screenHeight = HEIGHT * 10;

    InitWindow(screenWidth, screenHeight, "CHIP-8 Emulator");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        RenderDisplay(display);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}