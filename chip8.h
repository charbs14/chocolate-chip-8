#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct chip8 {

	uint8_t mem[4096];
	bool display[64 * 32];
	uint16_t pc;
	uint16_t i;
	uint16_t stack[128];
	uint8_t delay;
	uint8_t sound;
	uint8_t V[16];

} chip8;
	
	void load_rom(char* file);
	bool** chip8_cycle();

	
#endif
