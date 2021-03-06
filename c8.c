#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BLOCK 10
#define WIDTH 640
#define HEIGHT 320
#define BPP 4
#define DEPTH 32

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
    SDL_Surface *screen;
    SDL_Event event;
    int keypress;
    int x;
    int y;
    int c;
} Display;

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
    unsigned char display[64][32];
    unsigned char dt;
    unsigned char st;
    int romfd;
    int draw;
} c8;

void setpixel(Display *display, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    Uint32 *pixmem32;
    Uint32 colour;

    colour = SDL_MapRGB(display->screen->format, r, g, b);
    pixmem32 = (Uint32 *)display->screen->pixels + y + x;
    *pixmem32 = colour;
}

int DrawScreen(Display *display, int x, int y, int c)
{
    int ytimesw;
    int blockY;
    int blockX;
    x = x % 64 * BLOCK;
    y = y % 32 * BLOCK;

    if (c == 1)
    {
        c = 128;
    }
    else
    {
        c = 0;
    }

    if (SDL_MUSTLOCK(display->screen))
    {
        if (SDL_LockSurface(display->screen) < 0)
            return 1;
    }

    for (blockY = 0; blockY < BLOCK; blockY++)
    {
        ytimesw = y * display->screen->pitch / BPP;
        for (blockX = 0; blockX < BLOCK; blockX++)
        {
            setpixel(display, blockX + x, (blockY * (display->screen->pitch / BPP)) + ytimesw, c, c, c);
        }
    }

    return 0;
}

int decTimers(c8 *c8)
{
    if (c8->dt > 0)
    {
        c8->dt = c8->dt - 1;
    }

    if (c8->st > 0)
    {
        c8->st = c8->st - 1;
    }

    return 0;
}

int clearDisplay(Display *display)
{
    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 64; x++)
        {
            DrawScreen(display, x, y, 0);
        }
    }

    if (SDL_MUSTLOCK(display->screen))
        SDL_UnlockSurface(display->screen);
    SDL_Flip(display->screen);

    return 0;
}

int ClearDisplay(Display *display)
{
    int x, y;

    for (y = 0; y < 32; y++)
    {
        for (x = 0; x < 64; x++)
        {
            DrawScreen(display, x, y, 0);
        }
    }

    if (SDL_MUSTLOCK(display->screen))
        SDL_UnlockSurface(display->screen);
    SDL_Flip(display->screen);

    return 0;
}

int UpdateGraphics(c8 *c8, Display *display)
{
    int x, y;

    ClearDisplay(display);

    for (y = 0; y < 32; y++)
    {
        for (x = 0; x < 64; x++)
        {
            if (c8->display[x][y] != 0)
            {
                DrawScreen(display, x, y, 1);
            }
        }
    }

    if (SDL_MUSTLOCK(display->screen))
        SDL_UnlockSurface(display->screen);
    SDL_Flip(display->screen);

    return 0;
}

int InitScreen(Display *display)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        return 1;

    if (!(display->screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_HWSURFACE)))
    {
        SDL_Quit();
        return 1;
    }

    return 0;
}

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

    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 64; x++)
        {
            c8->display[x][y] = 0;
        }
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

    /* Clear keypad */
    for (i = 0; i < 16; i++)
    {
        c8->key[i] = 0;
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
    fread(&c8->memory[0x200], sizeof(unsigned short), size, f);
    return 1;
}

int emulate_cycle(c8 *c8, Display *display)
{
    c8->opcode = c8->memory[c8->PC] << 8 | c8->memory[c8->PC + 1];

    unsigned char x, y, kk, n;
    unsigned short nnn;
    x = (c8->opcode & 0x0F00) >> 8;
    y = (c8->opcode & 0x00F0) >> 4;
    kk = (c8->opcode & 0x00FF);
    n = (c8->opcode & 0x000F);
    nnn = (c8->opcode & 0x0FFF);

    int opfound = 0;
    int i, tmp;

    unsigned short xcoord = 0;
    unsigned short ycoord = 0;
    unsigned short height = 0;
    unsigned short pixel;

    switch (c8->opcode & 0xF000)
    {
    case 0x1000:
        c8->PC = c8->opcode & 0x0FFF;

        opfound = 1;
        break;

    case 0xA000:
        c8->I = c8->opcode & 0x0FFF;
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;

    case 0x4000:
        if (c8->V[x] != kk)
        {
            c8->PC = c8->PC + 4;
        }
        else
        {
            c8->PC = c8->PC + 2;
        }
        opfound = 1;
        break;

    case 0xC000:
        c8->V[x] = 9 & kk;
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;
    case 0xB000:
        c8->PC = (nnn) + c8->V[0x0];
        break;

    case 0x6000:
        c8->V[x] = c8->opcode & 0x00FF;
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;

    case 0xD000:
        c8->V[0xF] = 0;
        height = c8->opcode & 0x000F;
        xcoord = c8->V[x];
        ycoord = c8->V[y];

        for (i = 0; i < height; i++)
        {
            pixel = c8->memory[c8->I + i];
            for (x = 0; x < 8; x++)
            {
                if ((pixel & (0x80 >> x)) != 0)
                {
                    if (c8->display[xcoord + x][ycoord + i] == 1)
                        c8->V[0xF] = 1;
                    c8->display[xcoord + x][ycoord + i] ^= 1;
                }
            }
        }

        c8->draw = 1;
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;

    case 0x2000: /* Checked */
        c8->stack[c8->SP] = c8->PC;
        c8->SP++;
        c8->PC = c8->opcode & 0x0FFF;
        opfound = 1;
        break;

    case 0x5000:
        if (c8->V[x] == c8->V[y])
        {
            c8->PC += 4;
        }
        else
        {
            c8->PC += 2;
        }
        break;

    case 0x3000:
        if (c8->V[x] == kk)
        {
            c8->PC = c8->PC + 4;
        }
        else
        {
            c8->PC = c8->PC + 2;
        }
        opfound = 1;
        break;

    case 0x7000: /* Checked */
        c8->V[x] = c8->V[x] + kk;
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;
    }

    if (opfound == 1)
    {
        decTimers(c8);
        return 0;
    }

    switch (c8->opcode & 0xF0FF)
    {
    case 0xF00A:
        for (i = 0; i < 16; i++)
        {
            if (c8->key[i] != 0)
            {
                c8->V[x] = c8->key[i];
                c8->PC = c8->PC + 2;
            }
        }
        opfound = 1;
        break;

    case 0xF01E:
        c8->I = c8->I + c8->V[x];
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;

    case 0xF018:
        c8->st = c8->V[x];
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;

    case 0xF033:
        c8->memory[c8->I] = c8->V[x] / 100;
        c8->memory[c8->I + 1] = (c8->V[x] / 10) % 10;
        c8->memory[c8->I + 2] = (c8->V[x] / 1) % 10;

        c8->PC = c8->PC + 2;
        opfound = 1;
        break;

    case 0xE09E:
        if (c8->key[c8->V[x]] != 0)
        {
            c8->PC = c8->PC + 4;
        }
        else
        {
            c8->PC = c8->PC + 2;
        }
        opfound = 1;
        break;

    case 0xE0A1:
        if (c8->key[c8->V[x]] != 1)
        {
            c8->PC = c8->PC + 4;
        }
        else
        {
            c8->PC = c8->PC + 2;
        }
        opfound = 1;
        break;

    case 0xF007:
        c8->V[x] = c8->dt;
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;

    case 0xF015:
        c8->dt = c8->V[x];
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;

    case 0xF065:
        for (i = 0; i <= (x); i++)
        {
            c8->V[i] = c8->memory[c8->I + i];
            opfound = 1;
        }
        c8->PC = c8->PC + 2;
        break;

    case 0xF029:
        c8->I = c8->V[x] * 5;
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;

    case 0xF055:
        for (i = 0; i < x; i++)
        {
            c8->memory[c8->I + i] = c8->V[x];
        }
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;
    }

    if (opfound == 1)
    {
        decTimers(c8);
        return 0;
    }

    switch (c8->opcode & 0x00FF)
    {
    case 0x00E0:
        ClearDisplay(display);
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;

    case 0x00EE:
        c8->SP = c8->SP - 1;
        c8->PC = c8->stack[c8->SP];
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;
    }

    if (opfound == 1)
    {
        decTimers(c8);
        return 0;
    }

    switch (c8->opcode & 0xF00F)
    {
    case 0x8000:
        c8->V[x] = c8->V[y];
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;
    case 0x8002:
        c8->V[x] = c8->V[x] & c8->V[y];
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;
    case 0x8003:
        c8->V[x] = c8->V[x] ^ c8->V[y];
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;
    case 0x8004:
        tmp = c8->V[x];
        c8->V[x] = c8->V[x] + c8->V[y];

        if ((tmp + c8->V[y]) > 255)
        {
            c8->V[0xF] = 1;
        }
        else
        {
            c8->V[0xF] = 0;
        }

        c8->PC = c8->PC + 2;
        opfound = 1;
        break;

    case 0x8005:
        if (c8->V[x] > c8->V[y])
        {
            c8->V[0xF] = 1;
        }
        else
        {
            c8->V[0xF] = 0;
        }
        c8->V[x] = c8->V[x] - c8->V[y];

        c8->PC = c8->PC + 2;
        opfound = 1;
        break;
    case 0x800E:
        if ((c8->V[x] >> 8) == 1)
        {
            c8->V[0xF] = 1;
        }
        else
        {
            c8->V[0xF] = 0;
        }
        c8->V[x] = c8->V[x] * 2;
        c8->PC = c8->PC + 2;
        opfound = 1;
        break;

    case 0x9000:
        if (c8->V[x] != c8->V[y])
        {
            c8->PC = c8->PC + 4;
        }
        else
        {
            c8->PC = c8->PC + 2;
        }
        opfound = 1;
        break;
    }

    if (c8->dt > 0)
    {
        c8->dt = c8->dt - 1;
    }

    if (c8->st > 0)
    {
        c8->st = c8->st - 1;
    }

    if (opfound == 0)
    {
        printf("%x not found.\n", c8->opcode);
        exit(-1);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    c8 c8;

    Display display;

    SDL_Surface screen;

    SDL_Event event;

    display.screen = &screen;
    display.event = event;

    init(&c8);
    int quit = 0;

    if (InitScreen(&display) != 0)
        exit(-1);

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
        SDL_PollEvent(&event);
        switch (event.type)
        {
        case SDL_KEYUP:
            for (int i = 0; i < 16; i++)
            {
                c8.key[i] = 0;
            }
            break;
        }
        decTimers(&c8);
        emulate_cycle(&c8, &display);
        usleep(800);

        if (c8.draw)
        {
            c8.draw = 0;
            UpdateGraphics(&c8, &display);
        }

        switch (event.key.keysym.sym)
        {
        case SDLK_q:
            quit = 1;
            break;

        case SDLK_1:
            c8.key[0] = 1;
            break;

        case SDLK_2:
            c8.key[1] = 1;
            break;

        case SDLK_DOWN:
            c8.key[2] = 1;
            break;

        case SDLK_4:
            c8.key[3] = 1;
            break;

        case SDLK_LEFT:
            c8.key[4] = 1;
            break;

        case SDLK_a:
            c8.key[5] = 1;
            break;

        case SDLK_RIGHT:
            c8.key[6] = 1;
            break;

        case SDLK_8:
            c8.key[7] = 1;
            break;

        case SDLK_UP:
            c8.key[8] = 1;
            break;

        case SDLK_m:
            c8.key[9] = 1;
            break;

        case SDLK_0:
            c8.key[10] = 1;
            break;

        case SDLK_b:
            c8.key[11] = 1;
            break;

        case SDLK_c:
            c8.key[12] = 1;
            break;

        case SDLK_d:
            c8.key[13] = 1;
            break;

        case SDLK_e:
            c8.key[14] = 1;
            break;

        case SDLK_f:
            c8.key[15] = 1;
            break;
        default:
            break;
        }
    }
    return 0;
}
