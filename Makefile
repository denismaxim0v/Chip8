all:
	gcc -ggdb -Wall c8.c -o c8 -I /usr/include/SDL/ `sdl-config --cflags --libs` -std=c99

