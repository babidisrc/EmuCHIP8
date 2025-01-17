#include <stdlib.h>
#include <stdio.h>

// reference: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.2

// *codebuffer is a valid pointer to CHIP8 assembly code
int disassembleCHIP8(unsigned char *codebuffer, int pc) {
    unsigned char *code = &codebuffer[pc];

    int opbytes = 2;

    // combine 2 bytes in one 16-bit opcode
    int opcode = (code[0] << 8) | code[1];

    printf ("%04x: %04x -> ", pc, opcode);

    // check the first nibble (4 bits) of the opcode
    switch (opcode & 0xF000) { // isolate the first nibble
        case 0x1000: // jump to nnn, PC = nnn
            printf("JP 0x%03x", (opcode & 0x0FFF)); 
            break;
        case 0x2000: // call subroutine at nnn
            printf("CALL 0x%03x", (opcode & 0x0FFF)); 
            break;
        case 0x3000: // skip next instruction if Vx == kk
            printf("SE V%1x, 0x%02x", (opcode & 0x0F00) >> 8, (opcode & 0x00FF)); 
            break;
        case 0x4000: // skip next instruction if Vx != kk
            printf("SNE V%1x, 0x%02x", (opcode & 0x0F00) >> 8, (opcode & 0x00FF)); 
            break;
        case 0x5000: // skip next instruction if Vx != kk
            printf("SE V%1x, V%1x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8); 
            break;
        case 0x6000: // set Vx = kk
            printf("LD V%1x, 0x%02x", (opcode & 0x0F00) >> 8, (opcode & 0x00FF)); 
            break;
        case 0x7000: // set Vx = Vx + kk
            printf("ADD V%1x, 0x%02x", (opcode & 0x0F00) >> 8, (opcode & 0x00FF)); 
            break;
        case 0x8000:
            switch (opcode & 0x000F) { // isolate the last nibble
                case 0x0000: // set Vx = Vy
                    printf("LD V%1x, V%1x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0001: // set Vx OR Vy
                    printf("OR V%1x, V%1x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0002: // set Vx AND Vy
                    printf("AND V%1x, V%1x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0003: // set Vx XOR Vy
                    printf("XOR V%1x, V%1x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0004: // set Vx = Vx + Vy, set VF = carry
                    printf("ADD V%1x, V%1x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0005: // set Vx = Vx - Vy, set VF = NOT borrow
                    printf("SUB V%1x, V%1x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0006: // Vx = Vx SHR 1
                    printf("SHR V%1x {, V%1x}", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x0007: // set Vx = Vy - Vx, set VF = NOT borrow
                    printf("SUBN V%1x, V%1x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                case 0x000E: // Vx = Vx SHL 1
                    printf("SHL V%1x {, V%1x}", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8);
                    break;
                default: printf("UNKNOWN"); break;
            } 
            break;
        case 0x9000: // skip next instruction if Vx != Vy
            printf("SNE V%1x, V%1x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8); 
            break;
        case 0xA000: // set I = nnn
            printf("LD I, 0x%03x", (opcode & 0x0FFF)); 
            break;
        case 0xB000: // jump to location nnn + V0
            printf("JP V0, 0x%03x", (opcode & 0x0FFF)); 
            break;
        case 0xC000: // set Vx = random byte AND kk
            printf("RND V%1x, 0x%03x", (opcode & 0x0F00) >> 8, (opcode & 0x0FFF)); 
            break;
        case 0xD000: // display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
            printf("DRW V%1x, V%1x, %1x", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 8, (opcode & 0x00F) >> 8); 
            break;
        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x009E: // skip next instruction if key with the value of Vx is pressed
                    printf("SKP V%1x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x00A1: // skip next instruction if key with the value of Vx is NOT pressed
                    printf("SKNP V%1x", (opcode & 0x0F00) >> 8);
                    break;
                default: printf("UNKNOWN"); break;
            } 
            break; 
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007: // set Vx = delay timer value
                    printf("LD V%1x, DT", (opcode & 0x0F00) >> 8);
                    break;
                case 0x000A: // wait for a key press, store the value of the key in Vx
                    printf("LD V%1x, K", (opcode & 0x0F00) >> 8);
                    break;
                case 0x0015: // set delay timer = Vx
                    printf("LD DT, V%1x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x0018: // set sound timer = Vx
                    printf("LD ST, V%1x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x001E: // set I = I + Vx
                    printf("ADD I, V%1x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x0029: // set I = location of sprite for digit Vx
                    printf("LD F, V%1x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x0033: // store BCD representation of Vx in memory locations I, I+1, and I+2
                    printf("LD B, V%1x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x0055: // store registers V0 through Vx in memory starting at location I
                    printf("LD [I], V%1x", (opcode & 0x0F00) >> 8);
                    break;
                case 0x0065: // read registers V0 through Vx from memory starting at location I
                    printf("LD V%1x, [I]", (opcode & 0x0F00) >> 8);
                    break;
                default: printf("UNKNOWN"); break;
            } 
            break; 
        case 0x0000: // instructions starting with 0x0
            switch (opcode) {
                case 0x00E0: printf("CLS"); break; // clear display
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
    if (argc != 2) fatal("Usage: ./EmuCHIP8 <romfile> ");

    FILE *f= fopen(argv[1], "rb");    
    if (f == NULL) fatal("error: Couldn't open file");     

    // get the file size and read it into a memory buffer    
    fseek(f, 0L, SEEK_END);    
    int fsize = ftell(f);    
    fseek(f, 0L, SEEK_SET);

    unsigned char *buffer = malloc(fsize);

    fread(buffer, fsize, 1, f);
    fclose(f);

    int pc = 0;

    while (pc < fsize) {
        pc += disassembleCHIP8(buffer, pc);
    }
}