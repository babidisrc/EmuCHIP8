#include "raylib.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define WIDTH 64
#define HEIGHT 32

uint8_t display[HEIGHT * WIDTH] = {0}; // 64x32

int onNextKeyPress = -1;

int GetKey() {
	int key = GetKeyPressed();
    if (key == (KEY_ONE)) return 0x1; 
    if (key == (KEY_TWO)) return 0x2; 
    if (key == (KEY_THREE)) return 0x3;
    if (key == (KEY_FOUR)) return 0xC;
    if (key == (KEY_Q)) return 0x4;
    if (key == (KEY_W)) return 0x5;
    if (key == (KEY_E)) return 0x6;
    if (key == (KEY_R)) return 0xD;
    if (key == (KEY_A)) return 0x7;
    if (key == (KEY_S)) return 0x8;
    if (key == (KEY_D)) return 0x9;
    if (key == (KEY_F)) return 0xE;
    if (key == (KEY_Z)) return 0xA;
    if (key == (KEY_X)) return 0x0;
    if (key == (KEY_C)) return 0xB;
    if (key == (KEY_V)) return 0xF;

	if (onNextKeyPress != -1 && key) {
		onNextKeyPress = key;
		onNextKeyPress = -1;
	}

    return -1; 
}

void setPixel(uint8_t *d, int x, int y) {
	// pixel wrap around to the opposite side if outside of bounds
	if (x > WIDTH) x -= WIDTH;
	else if (x < 0) x += WIDTH;
	if (y > HEIGHT)	y -= HEIGHT;
	else if (y < 0) y += HEIGHT;
	
	int pixelLoc = x + (y * WIDTH);

	d[pixelLoc] ^= 1; // sprites are XORed onto display
}

void clear(uint8_t *d) {
	memset(d, 0, HEIGHT * WIDTH);
}

void render(uint8_t d[WIDTH * HEIGHT]) {
	const int scale = 10;
	ClearBackground(BLACK); // clean screen

	for (int i = 0; i < WIDTH * HEIGHT; i++) {
		int x = (i % WIDTH) * scale;
		int y = floor(i / HEIGHT) * scale;

		if (display[i]) {
			DrawRectangle(x, y, scale, scale, WHITE);
		}
	}
}

void testRender() {
    setPixel(display, 0, 0);
    setPixel(display, 5, 2);
}

void play(int frequency) {
	
}

int main(int argc, char *argv[]) {
    const int screenWidth = WIDTH * 10;
    const int screenHeight = HEIGHT * 10;

    InitWindow(screenWidth, screenHeight, "CHIP-8 Emulator");
    InitAudioDevice();
	
	SetTargetFPS(60);

	testRender();

    while (!WindowShouldClose()) {
        BeginDrawing();
		render(display);
		
        EndDrawing();
    }

	CloseAudioDevice();
    CloseWindow();
    return 0;
}