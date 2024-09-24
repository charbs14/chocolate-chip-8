#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_timer.h>
#include <SDL_image.h>
#include <time.h>
#include "chip8.h"

void init_chip8(chip8* chip){

	uint8_t font[80] = {	

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
	
	memset(chip->mem, 0, sizeof(chip->mem));
	memset(chip->stack, 0, sizeof(chip->stack));
	memset(chip->V, 0, sizeof(chip->V));
	
	memcpy(chip->mem, font, sizeof(font));

	chip->pc = 0x200;
	chip->i = 0;
	chip->delay = 60;
	chip->sound = 0;

	return;

}

void load_rom(chip8 *chip, char* file){

	FILE* rom;

	rom = fopen(file, "rb");//open file to read binary
				
	if(rom == NULL){
		printf("Failed to open ROM\n");
		exit(EXIT_FAILURE);
	}
	
	fseek(rom, 0, SEEK_END);
	int rom_size = ftell(rom);

	if(rom_size + 0x200 > 4096){
		printf("Rom exceedes 4096kb mem size");
		exit(EXIT_FAILURE);
	}
	
	rewind(rom);
	fread(&(chip->mem[0x200]),sizeof(uint8_t),rom_size, rom);
	fclose(rom);

	return;
}

uint16_t fetch(chip8 *chip){
	uint16_t instruction = ((chip->mem[chip->pc] << 8) | (0x00FF & chip->mem[(chip->pc)+1]));
	chip->pc += 2;
	
	return instruction; 
}

bool decodeAndExecute(chip8 *chip, uint16_t opcode){	

	uint8_t nib1 = (opcode & 0xF000) >> 12;
	uint8_t nib2 = (opcode & 0x0F00) >> 8;
	uint8_t nib3 = (opcode & 0x00F0) >> 4;
	uint8_t nib4 = (opcode & 0x000F);

	bool updated = 0;

	switch(nib1){
		case 0x0://clear screen
			if(opcode == 0x00E0){
				memset(chip->display, 0, sizeof(chip->display));
				updated = 1;
			}

			break;

		case 0x1://jump instruction
			chip->pc = (opcode & 0x0FFF);
			break;

		case 0x6:
			chip->V[nib2] = (opcode & 0x00FF);
			break;
		case 0x7:
			chip->V[nib2] += (opcode & 0x00FF);
			break;

		case 0xA:
			chip->i = (opcode & 0x0FFF);
			break;
		case 0xD:{
			uint8_t yPos = chip->V[nib3] & 31;
			chip->V[15] = 0;

			for(uint8_t row = 0; row < nib4; row++){
				uint8_t currByte = chip->mem[chip->i + row];
				uint8_t getPixel = 0x80;
				uint8_t xPos = chip->V[nib2] & 63;

				for(uint8_t pixel = 0; pixel < 8; pixel++){
					uint16_t index = (64 * yPos) + xPos;

					if(chip->display[index] && ((getPixel & currByte)) >> (7 - pixel)){
						chip->V[15] = 1;
					}

					chip->display[index] = chip->display[index] ^ (getPixel & currByte);

					if((xPos) == 63) 
						break;
					xPos++;
					getPixel = (getPixel >> 1);

				}

				if ( yPos == 31 ) 
					break;

				yPos++;
			}

			updated = 1;
			break;

		}

		default:
			 printf("Error: instruction not found");
			 exit(EXIT_FAILURE);
			 break;

			}
	return updated;
}

void updateScreen(SDL_Renderer *rend, SDL_Texture *tex, bool screen[64 * 32]){

	uint32_t display[64 * 32];

	for (int i = 0; i < (64 * 32); i++){
		display[i] = screen[i] ? 0xFFFFFFFF : 0x0; 
	}

	SDL_UpdateTexture(tex, NULL, display, 64 * sizeof( uint32_t ));


	SDL_RenderClear(rend);
	SDL_RenderCopy(rend,tex, NULL, NULL);
	SDL_RenderPresent(rend);

 	return;
}

int main ( int argc, char* argv[]){

	if ( argc != 2 ){
		printf("Incorrect usage: %s, <rom file>" , argv[0]);
		exit(EXIT_FAILURE);
	}

	chip8 chip;

	init_chip8(&chip);
	load_rom(&chip, argv[1]);

	printf("\nROM loaded!");

	if(SDL_Init(SDL_INIT_VIDEO) != 0){
		printf("error initializing SDL: %s\n", SDL_GetError());
		return 1;
	}

	unsigned int window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;

	SDL_Window *win = SDL_CreateWindow("chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, window_flags);

	printf("\nWindow Created!");



	if(!win){

		printf("Error creating SDL window%s\n",SDL_GetError() );		
		SDL_Quit();
		return 1;

	  }

	unsigned int render_flags = SDL_RENDERER_ACCELERATED;

	SDL_Renderer *rend = SDL_CreateRenderer(win, -1, render_flags);

	if(!rend){ printf("Error creating renderer: %s\n", SDL_GetError());
		SDL_DestroyWindow(win);
		SDL_Quit();
		return 1;
	}

	SDL_Texture *tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
	
	bool quit = 0;
	SDL_Event e;

	while(!quit){
		while(SDL_PollEvent(&e)){
			if(e.type == SDL_QUIT){
				quit = 1;
			}
		}
		
		uint16_t instruction = fetch(&chip);
		bool screenUpdated = decodeAndExecute(&chip, instruction);

		if(screenUpdated){
			updateScreen(rend, tex, chip.display);
		}

	}



	return 0; 
}
