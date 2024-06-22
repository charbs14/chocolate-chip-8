#ifndef CHIP8_H
#define CHIP8_H

#include <stdint>

	extern uint8_t mem[4096];
	extern uint8_t display[64 * 32];
	extern uint16_t pc;
	extern uint16_t I;
	extern uint16_t stack[128];
	extern uint8_t delay;
	extern uint8_t sound;
	extern uint8_t V[16];

	
	
#endif
