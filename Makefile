# Makefile for legbop 0.9 by Norbert de Jonge
#
# cppcheck --language=c --std=c99 --verbose legbop.c
# Also try the line below with clang instead of gcc.
#
all:
	gcc -O2 -Wno-unused-result -std=c99 -g -pedantic -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes legbop.c -o legbop `sdl2-config --cflags --libs` -lSDL2_ttf -lSDL2_image -lm
