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
  int x, y;

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
  FILE *f = fopen(*rom, "r");
  if(f == NULL) {
    fprintf(stderr, "can't open file");
    return 0;
  }
  fseek(f, 0, SEEK_END);
  int size = ftell(f);
  fseek(f, 0, SEEK_SET);
  fread(c8->memory[0x200], sizeof(uint16_t), size, f);
  return 1;
}

int emulate_cycle(c8 * c8)
{
  c8->opcode = c8->memory[c8->PC] << 8 | c8->memory[c8->PC + 1];
  c8->PC += 2;

  unsigned char x, y, kk, n;
  x = (c8->opcode & 0x0F00) >>8;
  y = (c8->opcode & 0x00F0) >>4;
  kk = (c8->opcode & 0x00FF);
  n = (c8->opcode & 0x000F);

  printf("%x, x: %x, y: %x, kk: %x, n: %x \n", c8->opcode, x, y, kk, n);
}

int main(int argc, char *argv[])
{
  c8 c8;
  init(&c8);
  int quit = 0;
  printf("%x\n", c8.PC);
  printf("%x\n", c8.I);

  if (argc == 2)
  {
    load_rom(argv[1], &c8);
  }
  else
  {
    exit(-1);
  }
  while(quit != 1) {
    emulate_cycle(&c8);
  }
  return 0;
}
