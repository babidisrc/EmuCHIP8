#include "raylib.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

// references: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.2
//             https://tobiasvl.github.io/blog/write-a-chip-8-emulator/#prerequisites

#define WIDTH 64
#define HEIGHT 32

#define MEMORY_SIZE 4096
#define PROGRAM_START 0x200 

#define CPU_FREQUENCY 500  // Hz
#define TIMER_FREQUENCY 60 // Hz 
#define DISPLAY_FREQUENCY 60 // Hz

uint8_t memory[MEMORY_SIZE];  

uint16_t stack[16] = {0};
uint8_t sp = 0; // stack pointer

unsigned char keypad[16] = {0};

uint8_t display[HEIGHT * WIDTH] = {0}; // 64x32

uint8_t stopLoop = 0; // stop looping (e.g. IBM Logo)

// REGISTERS
uint8_t V[16] = {0}; 
uint8_t soundTimer = 0, delayTimer = 0;
uint16_t I = 0;

// Built-in font
unsigned char fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

// keymaps
int keymappings[16] = {
    KEY_ONE,    // 0x0 (1)
    KEY_TWO,    // 0x1 (2)
    KEY_THREE,  // 0x2 (3)
    KEY_FOUR,   // 0x3 (4)
    KEY_Q,      // 0x4 (Q)
    KEY_W,      // 0x5 (W)
    KEY_E,      // 0x6 (E)
    KEY_R,      // 0x7 (R)
    KEY_A,      // 0x8 (A)
    KEY_S,      // 0x9 (S)
    KEY_D,      // 0xA (D)
    KEY_F,      // 0xB (F)
    KEY_Z,      // 0xC (Z)
    KEY_X,      // 0xD (X)
    KEY_C,      // 0xE (C)
    KEY_V       // 0xF (V)
};

void initializeMemory() {
    memset(memory, 0, MEMORY_SIZE);

    // load fonts into memory
    memcpy(memory, fontset, sizeof(fontset));
}

void initializeRegisters() {
    memset(V, 0, sizeof(V)); 
    soundTimer = 0;
    delayTimer = 0;
    I = 0;
}

void pushStack(uint16_t addr) {
    if (sp < 16) {
        stack[sp++] = addr;
    } else {
        // handle stack overflow
    }
}

uint16_t popStack() {
    if (sp > 0) {
        return stack[--sp];
    } else {
        // handle stack underflow
        return 0;
    }
}

void keyHandler(unsigned char *kpad) {
    // updating the keypad with the current state
    for (int keycode = 0; keycode < 16; keycode++) {
        kpad[keycode] = IsKeyDown(keymappings[keycode]);
    }
}

void setPixel(uint8_t *d, int x, int y, int n) {
	V[0xF] = 0; // VF = 0
    
    // pixel wrap around to the opposite side if outside of bounds
	if (x >= WIDTH) x -= WIDTH;
	else if (x < 0) x += WIDTH;
	if (y >= HEIGHT) y -= HEIGHT;
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
    unsigned char *code = &codebuffer[*pc - PROGRAM_START];

    int opbytes = 2;

    // combine 2 bytes in one 16-bit opcode
    int opcode = (code[0] << 8) | code[1];

    // Vx register, we are basically "grabbing" the x present in some
    unsigned short x = (opcode & 0x0F00) >> 8;

    // Vy register, we are basically "grabbing" the y present in some
    unsigned short y = (opcode & 0x00F0) >> 4;

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
            printf("CALL 0x%03x\n", (opcode & 0x0FFF)); 
            pushStack(*pc + opbytes); // push address of next instruction
            *pc = (opcode & 0x0FFF);
            return 0;
            break;
        case 0x3000: // skip next instruction if Vx == kk
            printf("SE V%01x, 0x%02x", x, (opcode & 0x00FF)); 
            if (V[x] == (opcode & 0x00FF))
                *pc += opbytes;
            break;
        case 0x4000: // skip next instruction if Vx != kk
            printf("SNE V%01x, 0x%02x", x, (opcode & 0x00FF)); 
            if (V[x] != (opcode & 0x00FF))
                *pc += opbytes;
            break;
        case 0x5000: // skip next instruction if Vx == Vy
            printf("SE V%01x, V%1x", x, y); 
            if (V[x] == V[y])
                *pc += opbytes;
            break;
        case 0x6000: // set Vx = kk
            printf("LD V%01x, 0x%02x", x, (opcode & 0x00FF)); 
            V[x] = (opcode & 0x00FF);
            break;
        case 0x7000: // set Vx = Vx + kk
            printf("ADD V%01x, 0x%02x", x, (opcode & 0x00FF)); 
            V[x] = (V[x] + (opcode & 0x00FF)) & 0xFF;
            break;
        case 0x8000:
            switch (opcode & 0x000F) { // isolate the last nibble
                case 0x0000: // set Vx = Vy
                    printf("LD V%01x, V%01x", x, y);
                    V[x] = V[y];
                    break;
                case 0x0001: // set Vx OR Vy
                    printf("OR V%01x, V%01x", x, y);
                    V[x] |= V[y];
                    break;
                case 0x0002: // set Vx AND Vy
                    printf("AND V%01x, V%01x", x, y);
                    V[x] &= V[y];
                    break;
                case 0x0003: // set Vx XOR Vy
                    printf("XOR V%01x, V%01x", x, y);
                    V[x] ^= V[y];
                    break;
                case 0x0004: // set Vx = Vx + Vy, set VF = carry
                    printf("ADD V%01x, V%01x", x, y);
                    V[x] += V[y];
                    if (V[x] > 0xFF) {
                        V[0xF] = 1;
                    } else {
                        V[0xF] = 0;
                    }
                    break;
                case 0x0005: // set Vx = Vx - Vy, set VF = NOT borrow
                    printf("SUB V%01x, V%01x", x, y);
                    V[x] -= V[y];
                    if (V[x] > V[y]) {
                        V[0xF] = 1;
                    } else {
                        V[0xF] = 0;
                    }
                    break;
                case 0x0006: // Vx = Vx SHR 1
                    printf("SHR V%01x {, V%01x}", x, y);
                    V[0xF] = V[x] & 0x01;
                    V[x] = V[x] >> 1;
                    break;
                case 0x0007: // set Vx = Vy - Vx, set VF = NOT borrow
                    printf("SUBN V%01x, V%01x", x, y);
                    if (V[y] > V[x]) {
                        V[0xF] = 1;
                    } else {
                        V[0xF] = 0;
                    }
                    break;
                case 0x000E: // Vx = Vx SHL 1
                    printf("SHL V%01x {, V%01x}", x, y);
                    uint8_t VX = V[x];
                    V[0xF] = VX & 0x01;
                    V[x] = VX << 1;
                    break;
                default: printf("UNKNOWN"); break;
            } 
            break;
        case 0x9000: // skip next instruction if Vx != Vy
            printf("SNE V%01x, V%01x", x, y); 
            if (V[x] != V[y])
                *pc += opbytes;
            break;
        case 0xA000: // set I = nnn
            printf("LD I, 0x%03x", (opcode & 0x0FFF));
            I = (opcode & 0x0FFF);
            break;
        case 0xB000: // jump to location nnn + V0
            printf("JP V0, 0x%03x", (opcode & 0x0FFF)); 
            *pc = (opcode & 0x0FFF) + V[0];
            return 0;
            break;
        case 0xC000: // set Vx = random byte (0-255) AND kk
            printf("RND V%01x, 0x%03x", x, opcode & 0x00FF); 
            V[x] = (rand() % 256) & (opcode & 0x00FF);
            break;
        case 0xD000: // display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
            printf("DRW V%01x, V%01x, %01x", x, y, (opcode & 0x00F)); 
            setPixel(display, V[x], V[y], (opcode & 0x00F));
            break;
        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x009E: // skip next instruction if key with the value of Vx is pressed
                    printf("SKP V%01x", x);
                    if (keypad[V[x]]) {
                        *pc += opbytes;
                    }
                    break;
                case 0x00A1: // skip next instruction if key with the value of Vx is NOT pressed
                    printf("SKNP V%01x", x);
                    if (!keypad[V[x]]) {
                        *pc += opbytes;
                    }
                    break;
                default: printf("UNKNOWN"); break;
            } 
            break; 
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007: // set Vx = delay timer value
                    printf("LD V%01x, DT", x);
                    V[x] = delayTimer;
                    break;
                case 0x000A: // wait for a key press, store the value of the key in Vx
                    printf("LD V%01x, K", x);
                    for (int i = 0; i < 16; i++) {
                        if (keypad[i]) {
                            V[x] = i;
                            pc += opbytes;
                            break;
                        }
                    }
                    break;
                case 0x0015: // set delay timer = Vx
                    printf("LD DT, V%01x", x);
                    delayTimer = V[x];
                    break;
                case 0x0018: // set sound timer = Vx
                    printf("LD ST, V%01x", x);
                    soundTimer = V[x];
                    break;
                case 0x001E: // set I = I + Vx
                    printf("ADD I, V%01x", x);
                     uint16_t newI = I + V[x]; // Soma I e VX
                    if (newI > 0x0FFF) { // if overflow
                        V[0xF] = 1; 
                    } else {
                        V[0xF] = 0; 
                    }
                    I = newI; 
                    break;
                case 0x0029: // set I = location of sprite for digit Vx
                    printf("LD F, V%01x", x);
                    int8_t digit = V[x] & 0x0F;
                    I = digit * 5;
                    break;
                case 0x0033: // store BCD representation of Vx in memory locations I, I+1, and I+2
                    printf("LD B, V%01x", x);
                    memory[I] = V[x] / 100;
                    memory[I + 1] = (V[x] % 100) / 10;
                    memory[I + 2] = V[x] % 10;
                    break;
                case 0x0055: // store registers V0 through Vx in memory starting at location I
                    printf("LD [I], V%01x", x);
                    for (int reg = 0; reg <= x; reg++) {
                        memory[I + reg] = V[reg];
                    }
                    break;
                case 0x0065: // read registers V0 through Vx from memory starting at location I
                    printf("LD V%01x, [I]", x);
                    for (int reg = 0; reg <= x; reg++) {
                        V[reg] = memory[I + reg];
                    }
                    break;
                default: printf("UNKNOWN"); break;
            } 
            break; 
        case 0x0000:
            switch (opcode) {
                case 0x00E0: // clear display
                    printf("CLS\n"); 
                    clear(display);
                    break; 
                case 0x00EE: 
                    printf("RET\n"); 
                    *pc = popStack();
                    return 0;
                    break; // return from a subroutine
                default: 
                    printf("SYS 0x%03x\n", (opcode & 0x0FFF)); 
                    break; // jump to a machine code routine at nnn - ignored by modern interpreters
            }
            break;
        default: printf("UNKNOWN"); break;
    }

    printf("\n");

    return opbytes;
}

void emulate(int fsize) {
    const double cpuTickInterval = 1.0 / CPU_FREQUENCY;
    const double timerTickInterval = 1.0 / TIMER_FREQUENCY;
    const double displayTickInterval = 1.0 / DISPLAY_FREQUENCY;
    
    double lastTime = GetTime();
    double cpuAccumulator = 0.0;
    double timerAccumulator = 0.0;
    double displayAccumulator = 0.0;
    
    int pc = PROGRAM_START;

    while (!WindowShouldClose()) {
        double currentTime = GetTime();
        double frameTime = currentTime - lastTime;
        lastTime = currentTime;
        
        cpuAccumulator += frameTime;
        timerAccumulator += frameTime;
        displayAccumulator += frameTime;

        // keyboard handling
        keyHandler(keypad);
        
        while (cpuAccumulator >= cpuTickInterval) {
            if (stopLoop == 0) {
                int increment = disassembleCHIP8(&memory[PROGRAM_START], &pc, fsize);
                pc += increment;
            }
            cpuAccumulator -= cpuTickInterval;
        }
        
        // update timers
        while (timerAccumulator >= timerTickInterval) {
            if (delayTimer > 0) {
                delayTimer--;
            }
            if (soundTimer > 0) {
                soundTimer--;
            }
            timerAccumulator -= timerTickInterval;
        }
        
        // render screen
        if (displayAccumulator >= displayTickInterval) {
            BeginDrawing();
            render(display);
            EndDrawing();
            displayAccumulator -= displayTickInterval;
        }
    }
}

void fatal(char *message) {
    printf("%s\n", message);
    exit(1);
}

int main(int argc, char *argv[]) {
    srand(time(NULL)); 

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

    initializeRegisters();

    emulate(fsize);

	CloseAudioDevice();
    CloseWindow();

    return 0;
}