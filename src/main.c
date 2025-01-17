#include "raylib.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define WIDTH 64
#define HEIGHT 32

#define MEMORY_SIZE 4096
#define PROGRAM_START 0x200 

uint8_t memory[MEMORY_SIZE];  

// references: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.2
//             https://tobiasvl.github.io/blog/write-a-chip-8-emulator/#prerequisites

uint8_t display[HEIGHT * WIDTH] = {0}; // 64x32

int onNextKeyPress = -1;
uint8_t stopLoop = 0; // stop looping (e.g. IBM Logo)

// REGISTERS
uint8_t V[16]; 
uint8_t soundTimer, delayTimer;
uint16_t I;

void initializeMemory() {
    memset(memory, 0, MEMORY_SIZE);

    // Built-in font
    uint8_t digit0[] = {0xF0, 0x90, 0x90, 0x90, 0xF0}; // sprite digit 0
    memcpy(memory, digit0, sizeof(digit0));

    uint8_t digit1[] = {0x20, 0x60, 0x20, 0x20, 0x70}; // sprite digit 1
    memcpy(memory, digit1, sizeof(digit1));

    uint8_t digit2[] = {0xF0, 0x10, 0xF0, 0x80, 0xF0}; // sprite digit 2
    memcpy(memory, digit2, sizeof(digit2));

    uint8_t digit3[] = {0xF0, 0x10, 0xF0, 0x10, 0xF0}; // sprite digit 3
    memcpy(memory, digit3, sizeof(digit3));

    uint8_t digit4[] = {0x90, 0x90, 0xF0, 0x10, 0x10}; // sprite digit 4
    memcpy(memory, digit4, sizeof(digit4));

    uint8_t digit5[] = {0xF0, 0x80, 0xF0, 0x10, 0xF0}; // sprite digit 5
    memcpy(memory, digit5, sizeof(digit5));

    uint8_t digit6[] = {0xF0, 0x80, 0xF0, 0x90, 0xF0}; // sprite digit 6
    memcpy(memory, digit6, sizeof(digit6));

    uint8_t digit7[] = {0xF0, 0x10, 0x20, 0x40, 0x40}; // sprite digit 7
    memcpy(memory, digit7, sizeof(digit7));

    uint8_t digit8[] = {0xF0, 0x90, 0xF0, 0x90, 0xF0}; // sprite digit 8
    memcpy(memory, digit8, sizeof(digit8));

    uint8_t digit9[] = {0xF0, 0x90, 0xF0, 0x10, 0xF0}; // sprite digit 9
    memcpy(memory, digit9, sizeof(digit9));

    uint8_t letterA[] = {0xF0, 0x90, 0xF0, 0x90, 0x90}; // sprite letter A
    memcpy(memory, letterA, sizeof(letterA));

    uint8_t letterB[] = {0xE0, 0x90, 0xE0, 0x90, 0xE0}; // sprite letter B
    memcpy(memory, letterB, sizeof(letterB));

    uint8_t letterC[] = {0xF0, 0x80, 0x80, 0x80, 0xF0}; // sprite letter C
    memcpy(memory, letterC, sizeof(letterC));

    uint8_t letterD[] = {0xE0, 0x90, 0x90, 0x90, 0xE0}; // sprite letter D
    memcpy(memory, letterD, sizeof(letterD));

    uint8_t letterE[] = {0xF0, 0x80, 0xF0, 0x80, 0xF0}; // sprite letter E
    memcpy(memory, letterE, sizeof(letterE));

    uint8_t letterF[] = {0xF0, 0x80, 0xF0, 0x80, 0x80}; // sprite letter F
    memcpy(memory, letterF, sizeof(letterF));
}

void initializeRegisters() {
    memset(V, 0, sizeof(V)); 
    soundTimer = 0;
    delayTimer = 0;
    I = 0;
}

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

void setPixel(uint8_t *d, int x, int y, int n) {
	V[0xF] = 0; // VF = 0
    
    // pixel wrap around to the opposite side if outside of bounds
	if (x >= WIDTH) x -= WIDTH;
	else if (x < 0) x += WIDTH;
	if (y >= HEIGHT)	y -= HEIGHT;
	else if (y < 0) y += HEIGHT;

    for (int row = 0; row < n; row++) {
        uint8_t spriteByte = memory[I + row];
        for (int bit = 0; bit < 8; bit++) {
            if (spriteByte & (0x80 >> bit)) { // verify if bit is ON
                int pixelX = (x + bit) % WIDTH;  //  X - CHIP-8
                int pixelY = (y + row) % HEIGHT; // Y - CHIP-8

                // collision
                int pixelLoc = pixelX + (pixelY * WIDTH);
                if (d[pixelX + (pixelY * WIDTH)]) {
                    V[0xF] = 1;
                     
                }

                d[pixelLoc] ^= 1; // sprites are XORed onto display
            }
        }
    }
}

void clear(uint8_t *d) {
	memset(d, 0, HEIGHT * WIDTH);
}

void render(uint8_t d[WIDTH * HEIGHT]) {
	ClearBackground(BLACK);
    const int scale = 10;

    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        // (X,Y) - raylib screen
        int x = (i % WIDTH) * scale;
        int y = (i / WIDTH) * scale;

        DrawRectangle(x, y, scale, scale, d[i] ? WHITE : BLACK);
    }
}

// *codebuffer is a valid pointer to CHIP8 assembly code
int disassembleCHIP8(unsigned char *codebuffer, int *pc, int fsize) {
    unsigned char *code = &codebuffer[*pc];

    int opbytes = 2;
    int skipNext = opbytes + 2;

    // combine 2 bytes in one 16-bit opcode
    int opcode = (code[0] << 8) | code[1];

    printf ("%04x: %04x -> ", *pc, opcode);

    // check the first nibble (4 bits) of the opcode
    switch (opcode & 0xF000) { // isolate the first nibble
        case 0x1000: // jump to nnn, PC = nnn
            printf("JP 0x%03x", (opcode & 0x0FFF));
            if (*pc == (opcode & 0x0FFF)) {
                stopLoop = 1;
            }
            *pc = (opcode & 0x0FFF);
            
            return 0;
            break;
        case 0x2000: // call subroutine at nnn
            printf("CALL 0x%03x", (opcode & 0x0FFF)); 
            break;
        case 0x3000: // skip next instruction if Vx == kk
            printf("SE V%01x, 0x%02x", (opcode & 0x0F00) >> 8, (opcode & 0x00FF)); 
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
                return skipNext;
            break;
        case 0x4000: // skip next instruction if Vx != kk
            printf("SNE V%01x, 0x%02x", (opcode & 0x0F00) >> 8, (opcode & 0x00FF)); 
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
                return skipNext;
            break;
        case 0x5000: // skip next instruction if Vx == Vy
            printf("SE V%01x, V%1x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8); 
            if (V[(opcode & 0x0F00) >> 8] == V[((opcode & 0x00F0) >> 8)])
                return skipNext;
            break;
        case 0x6000: // set Vx = kk
            printf("LD V%01x, 0x%02x", (opcode & 0x0F00) >> 8, (opcode & 0x00FF)); 
            V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
            break;
        case 0x7000: // set Vx = Vx + kk
            printf("ADD V%01x, 0x%02x", (opcode & 0x0F00) >> 8, (opcode & 0x00FF)); 
            V[(opcode & 0x0F00) >> 8] = (V[(opcode & 0x0F00) >> 8] + (opcode & 0x00FF)) & 0xFF;
            break;
        case 0x8000:
            switch (opcode & 0x000F) { // isolate the last nibble
                case 0x0000: // set Vx = Vy
                    printf("LD V%01x, V%01x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0001: // set Vx OR Vy
                    printf("OR V%01x, V%01x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0002: // set Vx AND Vy
                    printf("AND V%01x, V%01x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0003: // set Vx XOR Vy
                    printf("XOR V%01x, V%01x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0004: // set Vx = Vx + Vy, set VF = carry
                    printf("ADD V%01x, V%01x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0005: // set Vx = Vx - Vy, set VF = NOT borrow
                    printf("SUB V%01x, V%01x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0006: // Vx = Vx SHR 1
                    printf("SHR V%01x {, V%01x}", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0007: // set Vx = Vy - Vx, set VF = NOT borrow
                    printf("SUBN V%01x, V%01x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x000E: // Vx = Vx SHL 1
                    printf("SHL V%01x {, V%01x}", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                default: printf("UNKNOWN"); break;
            } 
            break;
        case 0x9000: // skip next instruction if Vx != Vy
            printf("SNE V%01x, V%01x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8); 
            break;
        case 0xA000: // set I = nnn
            printf("LD I, 0x%03x", (opcode & 0x0FFF));
            I = (opcode & 0x0FFF);
            break;
        case 0xB000: // jump to location nnn + V0
            printf("JP V0, 0x%03x", (opcode & 0x0FFF)); 
            break;
        case 0xC000: // set Vx = random byte AND kk
            printf("RND V%01x, 0x%03x", (opcode & 0x0F00) >> 8, (opcode & 0x0FFF)); 
            break;
        case 0xD000: // display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
            printf("DRW V%01x, V%01x, %01x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8, (opcode & 0x00F) >> 8); 
            setPixel(display, V[(opcode & 0x0F00) >> 8], V[(opcode & 0x00F0) >> 4], (opcode & 0x00F));
            render(display);
            break;
        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x009E: // skip next instruction if key with the value of Vx is pressed
                    printf("SKP V%01x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x00A1: // skip next instruction if key with the value of Vx is NOT pressed
                    printf("SKNP V%01x", (opcode & 0x0F00) >> 8);
                    break;
                default: printf("UNKNOWN"); break;
            } 
            break; 
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007: // set Vx = delay timer value
                    printf("LD V%01x, DT", (opcode & 0x0F00) >> 8);
                    break;
                case 0x000A: // wait for a key press, store the value of the key in Vx
                    printf("LD V%01x, K", (opcode & 0x0F00) >> 8);
                    break;
                case 0x0015: // set delay timer = Vx
                    printf("LD DT, V%01x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x0018: // set sound timer = Vx
                    printf("LD ST, V%01x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x001E: // set I = I + Vx
                    printf("ADD I, V%01x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x0029: // set I = location of sprite for digit Vx
                    printf("LD F, V%01x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x0033: // store BCD representation of Vx in memory locations I, I+1, and I+2
                    printf("LD B, V%01x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x0055: // store registers V0 through Vx in memory starting at location I
                    printf("LD [I], V%01x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x0065: // read registers V0 through Vx from memory starting at location I
                    printf("LD V%01x, [I]", (opcode & 0x0F00) >> 8);
                    break;
                default: printf("UNKNOWN"); break;
            } 
            break; 
        case 0x0000: // instructions starting with 0x0
            switch (opcode) {
                case 0x00E0: // clear display
                    printf("CLS"); 
                    clear(display);
                    break; 
                case 0x00EE: printf("RET"); break; // return from a subroutine
                default: printf("SYS 0x%03x", (opcode & 0x0FFF)); break; // jump to a machine code routine at nnn - ignored by modern interpreters

            }
            break;
        default: printf("UNKNOWN"); break;
    }

    printf("\n");

    return opbytes;
}

void fatal(char *message) {
    printf("%s\n", message);
    exit(1);
}

int main(int argc, char *argv[]) {
    const int screenWidth = WIDTH * 10;
    const int screenHeight = HEIGHT * 10;

    InitWindow(screenWidth, screenHeight, "CHIP-8 Emulator");
    InitAudioDevice();
	
	SetTargetFPS(60);

	if (argc != 2) fatal("Usage: ./EmuCHIP8 <romfile> ");

    FILE *f= fopen(argv[1], "rb");    
    if (f == NULL) fatal("error: Couldn't open file");     

    // get the file size and read it into a memory buffer    
    fseek(f, 0L, SEEK_END);    
    int fsize = ftell(f);    
    fseek(f, 0L, SEEK_SET);

    fread(&memory[PROGRAM_START], fsize, 1, f);
    fclose(f);

    int pc = 0;

    initializeRegisters();

    while (!WindowShouldClose()) {
        BeginDrawing();
        if (stopLoop == 0 && pc < fsize) {
            pc += disassembleCHIP8(&memory[PROGRAM_START], &pc, fsize);
        }
        EndDrawing();
    }

	CloseAudioDevice();
    CloseWindow();
    return 0;
}