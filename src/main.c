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

#define CPU_FREQUENCY 540  // Hz
#define TIMER_FREQUENCY 60 // Hz 
#define DISPLAY_FREQUENCY 60 // Hz

#define NUM_REGISTERS 16
#define STACK_SIZE 16

#define FONTSET_START_ADDRESS 0x050

typedef struct CHIP8Registers {
    // REGISTERS
    uint8_t V[NUM_REGISTERS]; 
    uint8_t soundTimer;
    uint8_t delayTimer;
    uint16_t I;
} CHIP8Registers;

typedef struct CHIP8CPU {
    uint8_t memory[MEMORY_SIZE];
    uint16_t stack[STACK_SIZE];
    uint8_t sp; // stack pointer
    unsigned char keypad[16];
    uint8_t display[HEIGHT * WIDTH]; // 64x32
    CHIP8Registers registers;
} CHIP8CPU;

#define V cpu.registers.V
#define I cpu.registers.I

CHIP8CPU cpu;

uint8_t stopLoop = 0; // stop looping (e.g. IBM Logo)

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
    memset(cpu.memory, 0, MEMORY_SIZE);

    // load fonts into memory
    memcpy(&cpu.memory[FONTSET_START_ADDRESS], fontset, sizeof(fontset));
}

void initializeRegisters() {
    memset(V, 0, sizeof(V)); 
    cpu.registers.soundTimer = 0;
    cpu.registers.delayTimer = 0;
    I = 0;
}

void pushStack(uint16_t addr) {
    if (cpu.sp < 16) {
        cpu.stack[cpu.sp++] = addr;
    } else {
        // handle stack overflow
    }
}

uint16_t popStack() {
    if (cpu.sp > 0) {
        return cpu.stack[--cpu.sp];
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

    for (int row = 0; row < n; row++) {
        uint8_t spriteByte = cpu.memory[I + row];
        if(spriteByte == 0) continue;

        for (int bit = 0; bit < 8; bit++) {
            if (spriteByte & (0x80 >> bit)) { // verify if bit is ON

                int pixelX = (x + bit) % WIDTH;  //  X - CHIP-8
                int pixelY = (y + row) % HEIGHT; // Y - CHIP-8

                // collision
                int pixelLoc = pixelX + (pixelY * WIDTH);
                
                if (d[pixelLoc]) {
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

    // check the first nibble (4 bits) of the opcode
    switch (opcode & 0xF000) { // isolate the first nibble
        case 0x1000: // jump to nnn, PC = nnn
            if (*pc == (opcode & 0x0FFF)) {
                stopLoop = 1;
            }
            *pc = (opcode & 0x0FFF);

            return 0;
            break;

        case 0x2000: // call subroutine at nnn
            pushStack(*pc + opbytes); // push address of next instruction
            *pc = (opcode & 0x0FFF);
            return 0;
            break;

        case 0x3000: // skip next instruction if Vx == kk
            if (V[x] == (opcode & 0x00FF))
                *pc += opbytes;
            break;

        case 0x4000: // skip next instruction if Vx != kk
            if (V[x] != (opcode & 0x00FF))
                *pc += opbytes;
            break;

        case 0x5000: // skip next instruction if Vx == Vy
            if (V[x] == V[y])
                *pc += opbytes;
            break;

        case 0x6000: // set Vx = kk
            V[x] = (opcode & 0x00FF);
            break;

        case 0x7000: // set Vx = Vx + kk
            V[x] = (V[x] + (opcode & 0x00FF)) & 0xFF;
            break;

        case 0x8000:
            switch (opcode & 0x000F) { // isolate the last nibble
                case 0x0000: // set Vx = Vy
                    V[x] = V[y];
                    break;

                case 0x0001: // set Vx OR Vy
                    V[x] |= V[y];
                    break;

                case 0x0002: // set Vx AND Vy
                    V[x] &= V[y];
                    break;

                case 0x0003: // set Vx XOR Vy
                    V[x] ^= V[y];
                    break;

                case 0x0004: // set Vx = Vx + Vy, set VF = carry
                    uint16_t sum = V[x] + V[y];
                    V[0xF] = (sum > 0xFF) ? 1 : 0;
                    V[x] = sum & 0xFF;
                    break;

                case 0x0005: // set Vx = Vx - Vy, set VF = NOT borrow
                    V[0xF] = (V[x] >= V[y]) ? 1 : 0;
                    V[x] = (V[x] - V[y]) & 0xFF;
                    break;

                case 0x0006: // Vx = Vx SHR 1
                    V[0xF] = V[x] & 0x01;
                    V[x] = V[x] >> 1;
                    break;

                case 0x0007: // set Vx = Vy - Vx, set VF = NOT borrow
                    V[0xF] = (V[y] >= V[x]) ? 1 : 0;
                    V[x] = (V[y] - V[x]) & 0xFF;
                    break;

                case 0x000E: // Vx = Vx SHL 1
                    uint8_t VX = V[x];
                    V[0xF] = VX & 0x01;
                    V[x] = VX << 1;
                    break;
                default: printf("UNKNOWN"); break;
            } 
            break;

        case 0x9000: // skip next instruction if Vx != Vy
            if (V[x] != V[y])
                *pc += opbytes;
            break;

        case 0xA000: // set I = nnn
            I = (opcode & 0x0FFF);
            break;

        case 0xB000: // jump to location nnn + V0 
            *pc = (opcode & 0x0FFF) + V[0];
            return 0;
            break;

        case 0xC000: // set Vx = random byte (0-255) AND kk
            V[x] = (rand() % 256) & (opcode & 0x00FF);
            break;

        case 0xD000: // display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
            setPixel(cpu.display, V[x], V[y], (opcode & 0x00F));
            break;

        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x009E: // skip next instruction if key with the value of Vx is pressed
                    if (cpu.keypad[V[x]]) {
                        *pc += opbytes;
                    }
                    break;

                case 0x00A1: // skip next instruction if key with the value of Vx is NOT pressed
                    if (!cpu.keypad[V[x]]) {
                        *pc += opbytes;
                    }
                    break;

                default: printf("UNKNOWN"); break;
            } 
            break; 

        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007: // set Vx = delay timer value
                    V[x] = cpu.registers.delayTimer;
                    break;

                case 0x000A: // wait for a key press, store the value of the key in Vx
                    for (int i = 0; i < 16; i++) {
                        if (cpu.keypad[i]) {
                            V[x] = i;
                            pc += opbytes;
                            break;
                        }
                    }
                    break;

                case 0x0015: // set delay timer = Vx
                    cpu.registers.delayTimer = V[x];
                    break;

                case 0x0018: // set sound timer = Vx
                    cpu.registers.soundTimer = V[x];
                    break;

                case 0x001E: // set I = I + Vx
                     uint16_t newI = I + V[x]; // Soma I e VX
                    if (newI > 0x0FFF) { // if overflow
                        V[0xF] = 1; 
                    } else {
                        V[0xF] = 0; 
                    }
                    I = newI; 
                    break;

                case 0x0029: // LD F, Vx - set I to location of sprite for digit Vx
                    I = FONTSET_START_ADDRESS + (V[x] * 5);
                    break;
                
                case 0x0033: // store BCD representation of Vx in memory locations I, I+1, and I+2
                    cpu.memory[I] = V[x] / 100;
                    cpu.memory[I + 1] = (V[x] % 100) / 10;
                    cpu.memory[I + 2] = V[x] % 10;
                    break;

                case 0x0055: // store registers V0 through Vx in memory starting at location I
                    for (int reg = 0; reg <= x; reg++) {
                        cpu.memory[I + reg] = V[reg];
                    }
                    break;

                case 0x0065: // read registers V0 through Vx from memory starting at location I
                    for (int reg = 0; reg <= x; reg++) {
                        V[reg] = cpu.memory[I + reg];
                    }
                    break;

                default: printf("UNKNOWN"); break;
            } 
            break; 
        case 0x0000:
            switch (opcode) {
                case 0x00E0: // clear display
                    clear(cpu.display);
                    break; 

                case 0x00EE: 
                    *pc = popStack();
                    return 0;
                    break; // return from a subroutine
                
                default: printf("UNKNOWN"); break;
            }
            break;
        default: printf("UNKNOWN"); break;
    }

    return opbytes;
}

void emulate(int fsize) {
    const double timeStep = 1.0 / 60.0;
    double lastTime = GetTime();
    int pc = PROGRAM_START;

    while (!WindowShouldClose()) {
        double currentTime = GetTime();
        double frameTime = currentTime - lastTime;
        lastTime = currentTime;

        // keyboard handling
        keyHandler(cpu.keypad);

        int cpuCycles = (int)(frameTime * CPU_FREQUENCY);
        
        for(int i = 0; i < cpuCycles && !stopLoop; i++) {
            int increment = disassembleCHIP8(&cpu.memory[PROGRAM_START], &pc, fsize);
            pc += increment;
        }
        
        // update timers
        if(frameTime >= timeStep) {
            if(cpu.registers.delayTimer > 0) cpu.registers.delayTimer--;
            if(cpu.registers.soundTimer > 0) cpu.registers.soundTimer--;
        }
        
        // render screen
        BeginDrawing();
        render(cpu.display);
        EndDrawing();
    }
}

void fatal(char *message) {
    printf("%s\n", message);
    exit(1);
}

int loadROM(char rom[]) {
    FILE *f= fopen(rom, "rb");    
    if (f == NULL) fatal("error: Couldn't open file");     

    // get the file size and read it into a memory buffer    
    fseek(f, 0L, SEEK_END);    
    int fsize = ftell(f);    
    fseek(f, 0L, SEEK_SET);

    fread(&cpu.memory[PROGRAM_START], fsize, 1, f);
    fclose(f);

    return fsize;
}

int main(int argc, char *argv[]) {
    srand(time(NULL)); 

    const int screenWidth = WIDTH * 10;
    const int screenHeight = HEIGHT * 10;

    InitWindow(screenWidth, screenHeight, "CHIP-8 Emulator");
    InitAudioDevice();
	
	SetTargetFPS(60);

	if (argc != 2) fatal("Usage: ./EmuCHIP8 <romfile> ");

    initializeMemory();

    int fsize = loadROM(argv[1]);

    initializeRegisters();

    emulate(fsize);

	CloseAudioDevice();
    CloseWindow();

    return 0;
}