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

#define VERSION_TOGGLE 0

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
	memset(chip->display, 0, sizeof(chip->display));
	
	memcpy(chip->mem, font, sizeof(font));

	chip->pc = 0x200;
	chip->i = 0;
	chip->delay = 0;
	chip->sound = 0;
	chip->sp = &(chip->stack[0]);

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

bool decodeAndExecute(chip8 *chip, uint16_t opcode, uint8_t keypress){	

	uint8_t nib1 = (opcode & 0xF000) >> 12;
	uint8_t nib2 = (opcode & 0x0F00) >> 8;
	uint8_t nib3 = (opcode & 0x00F0) >> 4;
	uint8_t nib4 = (opcode & 0x000F);

	bool updated = 0;

	switch(nib1){
		case 0x0:
			if(nib4 == 0x0){ //clear screen
				memset(chip->display, 0, sizeof(chip->display));
				updated = 1;
				break;
			}else if(nib4 == 0xE){//pop from stack
				 chip->pc = *(chip->sp);
				 chip->sp--;
			}

			break;

		case 0x1://jump instruction
			chip->pc = (opcode & 0x0FFF);
			break;

		case 0x2://Call subroutine
			chip->sp++;
			*(chip->sp) = chip->pc;
			chip->pc = (opcode & 0x0FFF);
			break;

		case 0x3:
			if(chip->V[nib2] == (opcode & 0x00FF)){
				chip->pc += 2;
			}
			break;

		case 0x4:
			if(chip->V[nib2] != (opcode & 0x00FF)){
				chip->pc += 2;
			}
			break;

		case 0x5:
			if(chip->V[nib2] == chip->V[nib3]){
				chip->pc += 2;
			}
			break;

		case 0x6:
			chip->V[nib2] = (opcode & 0x00FF);
			break;

		case 0x7:
			chip->V[nib2] += (opcode & 0x00FF);
			break;
		
		case 0x9:
			if(chip->V[nib2] != chip->V[nib3]){
				chip->pc += 2;
			}

			break;
		case 0x8:
			switch(nib4){
				case 0x0:
					chip->V[nib2] = chip->V[nib3];
					break;

				case 0x1:
					chip->V[nib2] = chip->V[nib2] | chip->V[nib3];
					break;

				case 0x2:
					chip->V[nib2] = chip->V[nib2] & chip->V[nib3];
					break;

				case 0x3:
					chip->V[nib2] = chip->V[nib2] ^ chip->V[nib3];
					break;
				
				case 0x4:
					chip->V[0xF] = 0;
					uint32_t sum = chip->V[nib2] + chip->V[nib3];

					if( sum  > 255 )
						chip->V[0xF] = 1;

					chip->V[nib2] = sum & 0xFF;

					break;

				case 0x5:
					chip->V[0xF] = 0;
					if(chip->V[nib2] >= chip->V[nib3])
						chip->V[0xF] = 1;

					chip->V[nib2] = chip->V[nib2] - chip->V[nib3];
					break;

				case 0x6:
					if(VERSION_TOGGLE){
						chip->V[nib2] = chip->V[nib3];
					}
					chip->V[0xF] = 0b00000001 & chip->V[nib2];
					chip->V[nib2] = chip->V[nib2] >> 1;
					break;

				case 0x7:
					chip->V[0xF] = 0;
					if(chip->V[nib3] >= chip->V[nib2])
						chip->V[0xF] = 1;

					chip->V[nib2] = chip->V[nib3] - chip->V[nib2];

					break;

				case 0xE:
					if(VERSION_TOGGLE){
						chip->V[nib2] = chip->V[nib3];
					}
					chip->V[0xF] = chip->V[nib2] >> 7;
					chip->V[nib2] = chip->V[nib2] << 1;
					break;


			}

		case 0xA:
			chip->i = (opcode & 0x0FFF);
			break;
		
		case 0xB:
			chip->pc = (opcode & 0x0FFF) + chip->V[0];
			break;

		case 0xC:
			chip->V[nib2] = rand() & (opcode & 0x00FF);
			break;

		case 0xD:{
			uint32_t x = chip->V[nib2] & 63;
			uint32_t y = chip->V[nib3] & 31;
			chip->V[15] = 0;

			for(uint32_t row = 0; row < nib4; row++){
				uint32_t currByte = chip->mem[chip->i + row];
				uint32_t index = ((y + row) * 64) + x;
				uint32_t pixelGrabber = 0x80;
				for(uint32_t column= 0; column< 8; column++){
					uint32_t currPixel;
					if(chip->display[index])
						currPixel = 1;
					else
						currPixel = 0;

					uint32_t newPixel = (currByte & pixelGrabber) >> (7 - column);

					if(currPixel && newPixel){
						chip->V[0xF] = 1;
					}
					
					chip->display[index] = (currPixel ^ newPixel) ? 0xFFFFFFFF:0x0;
					
					index++;
					if((index - (64 * (y + row)))== 64)
						break;
					pixelGrabber = pixelGrabber >> 1;

				}

				if((y + row) == 31)
					break;

			}

			updated = 1;
			break;

		}
		
		case 0xE:
			switch( opcode	& 0x00FF ) {
				case 0x9E:
					if(keypress == chip->V[nib2])
						chip->pc += 2;
					break;
				case 0xA1:
					if(keypress != chip->V[nib2])
						chip->pc += 2;
					break;

			}
			break;
		case 0xF:
			switch( opcode & 0x00FF ){
				case 0x07:
					chip->V[nib2] = chip->delay;
					break;
				case 0x15:
					chip->delay = chip->V[nib2];
					break;
				case 0x18:
					chip->sound = chip->V[nib2];
					break;
				case 0x1E:
					chip->i += chip->V[nib2];
					break;
				case 0x0A:
					if(keypress == 169){
						chip->pc -= 2;
					}
					chip->V[nib2] = keypress;

					break;
				case 0x29:
					chip->i = 5 * (chip->V[nib2] & 0x0F);
					break;
				case 0x33:{
					int num = chip->V[nib2];
					  
					chip->mem[chip->i] = (num - (num % 100)) / 100;
					num -= chip->mem[chip->i] * 100;
					chip->mem[chip->i + 1] = (num - (num % 10)) / 10;
					num -= chip->mem[chip->i + 1] * 10;
					chip->mem[chip->i + 2] = num;
					  }
					
					break;
				case 0x55:
					for (int j = 0; j <= nib2; j++){
						chip->mem[chip->i + j] = chip->V[j];
					}
					break;
				case 0x65:
					for (int j = 0; j <= nib2; j++){
						chip->V[j] = chip->mem[chip->i + j];
					}
					break;

			}
			break;
			 

		default:
			 printf("Error: instruction not found");
			 exit(EXIT_FAILURE);
			 break;

			}
	return updated;
}

void updateScreen(SDL_Renderer *rend, SDL_Texture *tex, uint32_t screen[64 * 32]){


	SDL_UpdateTexture(tex, NULL, screen, 64 * sizeof( uint32_t ));


	SDL_RenderClear(rend);
	SDL_RenderCopy(rend,tex, NULL, NULL);
	SDL_RenderPresent(rend);

 	return;
}
uint8_t getKeyHex(SDL_Event *e){
	switch(e->key.keysym.scancode){
		case SDL_SCANCODE_0:
			return 0;
		case SDL_SCANCODE_1:
			return 1;
		case SDL_SCANCODE_2:
			return 2;
		case SDL_SCANCODE_3:
			return 3;
		case SDL_SCANCODE_4:
			return 4;
		case SDL_SCANCODE_5:
			return 5;
		case SDL_SCANCODE_6:
			return 6;
		case SDL_SCANCODE_7:
			return 7;
		case SDL_SCANCODE_8:
			return 8;
		case SDL_SCANCODE_9:
			return 9;
		case SDL_SCANCODE_A:
			return 10;
		case SDL_SCANCODE_B:
			return 11;
		case SDL_SCANCODE_C:
			return 12;
		case SDL_SCANCODE_D:
			return 13;
		case SDL_SCANCODE_E:
			return 14;
		case SDL_SCANCODE_F:
			return 15;
		default:
			return 169;

	}
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
	uint32_t elapsed = 0;

	while(!quit){
		uint8_t keypress = 169;
		uint32_t start = SDL_GetTicks();
		while(SDL_PollEvent(&e)){
			if(e.type == SDL_QUIT){
				quit = 1;
			}else if(e.type == SDL_KEYDOWN){
				keypress = getKeyHex(&e);
			}
		}
		
		uint16_t instruction = fetch(&chip);
		bool screenUpdated = decodeAndExecute(&chip, instruction, keypress);
		uint32_t end = SDL_GetTicks();
		elapsed += (end - start);
		if (elapsed >= (1000/60)) {
			if (chip.delay != 0)
				chip.delay--;
			elapsed = 0;
		}

		if(screenUpdated){
			updateScreen(rend, tex, chip.display);
		}

	}


	SDL_Delay(1000/60);
	return 0; 
}
