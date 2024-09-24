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

	void init_chip8(chip8 *chip);
	void load_rom(chip8 *chip, char* file);
	uint16_t fetch(chip8 *chip ); //fetch opcode from memory, return two word opcode
	bool decodeAndExecute(chip8 *chip, uint16_t opcode ); //executes process, returns 0 if display was not updaetd, otherwise returns 1
	
