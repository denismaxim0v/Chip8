#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

unsigned char fontset[80] =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

typedef struct
{
    unsigned short opcode;
    unsigned char memory[4096];
    unsigned char V[16];
    unsigned short I;
    unsigned short PC;
    unsigned short SP;
    unsigned short stack[16];
    unsigned char key[16];
    unsigned char ROM[4096];
    unsigned char display[32][64];
    unsigned char dt;
    unsigned char st;
    int romfd;
    int draw;
} c8;

int init(c8 *c8)
{
    int i;

    c8->PC = 0x200;
    c8->opcode = 0;
    c8->I = 0;
    c8->SP = 0;

    for (i = 0; i < 16; i++)
    {
        c8->V[i] = 0;
    }

    for (i = 0; i < 16; i++)
    {
        c8->stack[i] = 0;
    }

    for (i = 0; i < 4096; i++)
    {
        c8->memory[i] = 0;
    }

    for (i = 0; i < 80; i++)
    {
        c8->memory[i] = fontset[i];
    }

    c8->dt = 0;
    c8->st = 0;

    return 0;
}

int load_rom(char *rom, c8 *c8)
{
    FILE *f = fopen(rom, "r");
    if (f == NULL)
    {
        fprintf(stderr, "can't open file");
        return 0;
    }
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);
    fread(&c8->memory[0x200], sizeof(uint16_t), size, f);
    return 1;
}

int emulate_cycle(c8 *c8)
{
    c8->opcode = c8->memory[c8->PC] << 8 | c8->memory[c8->PC + 1];
    c8->PC += 2;

    unsigned char x, y, kk, n;
    unsigned short nnn;
    x = (c8->opcode & 0x0F00) >> 8;
    y = (c8->opcode & 0x00F0) >> 4;
    kk = (c8->opcode & 0x00FF);
    n = (c8->opcode & 0x000F);
    nnn = (c8->opcode & 0x0FFF);

    switch (c8->opcode & 0xF000)
    {
    case 0x0000:
        switch (c8->opcode & 0x00FF)
        {
        case 0x00E0: //TODO cls
            break;
        case 0x00EE:
            --c8->SP;
            c8->PC = c8->stack[c8->SP];
            break;
        }
        break;
    case 0x1000:
        c8->PC = nnn;
        break;
    case 0x2000:
        c8->stack[c8->SP] = c8->PC;
        ++c8->SP;
        c8->PC = nnn;
        break;
    case 0x3000:
        if (c8->V[x] == kk)
        {
            c8->PC += 2;
        }
        break;
    case 0x4000:
        if (c8->V[x] != kk)
        {
            c8->PC += 2;
        }
        break;
    case 0x5000:
        if (c8->V[x] == c8->V[y])
        {
            c8->PC += 2;
        }
        break;
    case 0x6000:
        c8->V[x] = kk;
        break;
    case 0x7000:
        c8->V[(x)] += kk;
        break;
    case 0x8000:
        switch (n)
        {
        case 0x0000:
            c8->V[x] = c8->V[y];
            break;
        case 0x0001:
            c8->V[x] = c8->V[x] | c8->V[y];
            break;
        case 0x0002:
            c8->V[x] = c8->V[x] & c8->V[y];
            break;
        case 0x0003:
            c8->V[x] = c8->V[x] ^ c8->V[y];
            break;
        case 0x0004:
        {
            int i = (int)(c8->V[x]) + (int)(c8->V[y]);
            if (i > 255)
            {
                c8->V[0xF] = 1;
            }
            else
            {
                c8->V[0xF] = 0;
            }
            c8->V[x] = i & 0xFF;
        }
        break;
        case 0x0005:
        {
            if (c8->V[x] > c8->V[y])
            {
                c8->V[0xF] = 1;
            }
            else
            {
                c8->V[0xF] = 0;
            }
            c8->V[x] = c8->V[x] - c8->V[y];
        }
        break;
        case 0x0006:
        {
            c8->V[0xF] = c8->V[x] & 1;
            c8->V[x] >>= 1;
        }
        break;
        case 0x0007:
        {
            if (c8->V[y] > c8->V[x])
            {
                c8->V[0xF] = 1;
            }
            else
            {
                c8->V[0xF] = 0;
            };
            c8->V[x] = c8->V[y] - c8->V[x];
        }
        break;
        case 0x000E:
        {
            c8->V[0xF] = c8->V[x] >> 7;
            c8->V[x] <<= 1;
        }
        break;
        default:
            printf("Op %x\n", c8->opcode);
        }
    case 0x9000:
    {
        if (c8->V[x] != c8->V[y])
        {
            c8->PC += 2;
        }
    }
    break;
    case 0xA000:
    {
        c8->I = nnn;
    }
    break;
    case 0xB000:
    {
        c8->PC = (nnn) + c8->V[0x0];
    }
    break;
    case 0xC000:
    {
        // TODO make it random here
        //c8->V[x] = 
    }
    break;
    case 0xD000:
    {
        // TODO drawing stuff here
    }
    break;
    // TODO key pressing stuff here
    default: printf("%x\n", c8->opcode);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    c8 c8;
    init(&c8);
    int quit = 0;

    if (argc == 2)
    {
        load_rom(argv[1], &c8);
    }
    else
    {
        exit(-1);
    }

    while (quit != 1)
    {
        emulate_cycle(&c8);
    }
    return 0;
}
