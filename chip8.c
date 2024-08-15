#ifndef CHIP8_C
#define CHIP8_C

#include "chip8.h"
#include <stdint.h>
#include <unistd.h>
#include <stdio>
#include <SDL.h>
#include <SDL_timer.h>
#include <SDL_image.h>
#include <time.h>

	uint8_t mem[4096] = {//4kb of mem, initialized with fonts 
		
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

	bool display[64 * 32];  //64 * 32 pixel display
					
					
	uint16_t pc = 0x200; //program counter, begins at 0x200 in mem since 
			     //interpreters were usually stored before that address
	
	uint16_t i = 0; //index register used to point at locations in memory

	uint16_t stack[128]; //stack for subroutines and functions
				   
	uint8_t delay = 60; //delay timer, decremented 60 times per sec until it reaches 0
			    
	uint8_t sound = 0; //sound timer which gives off beeping sound as long as its not 0
	
	uint8_t V[16]; //16 one byte general purpose registers. 
			     //V[15] or VF is used as a flag register

	void load_rom(char* file){
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
		fread(&mem[0x200],sizeof(uint8_t),rom_size, rom);
		fclose(rom);

		return;

	}
	bool emulate_cycle(){
		uint16_t instruction =  mem[pc+1] | (mem[pc] << 8); //instruction is two bytes from memory 
		pc += 2;

		//decode instruction, first break into nibbles
		uint8_t nib1 = (instruction & 0xF000) >> 12;
		uint8_t nib2 = (instruction & 0x0F00) >> 8;
		uint8_t nib3 = (instruction & 0x00F0) >> 4;
		uint8_t nib4 = (instruction & 0x000F);

		bool displayUpdated = 0;

		switch(nib1){
			case 0x0:
				if(instruction == 0x00E0){//clear screen 
					memset(display, 0, sizeof(display)); 	
				}
				
				break;

			case 0x1://jump instruction 
				pc = (instruction & 0x0FFF);
				break;
			case 0x6://set register value
				V[nib2] = (instruction & 0x00FF);	
				break;
			case 0x7://add immediate to register
				V[nib2] += (instruction & 0x00FF);
				break;
			case 0xA://set index register
				i = (instruction & 0x0FFF);		
				break;
			case 0xD://display
				 uint8_t xPos = V[nib2] & 63;
				 uint8_t yPos = V[nib3] & 31;
				 V[15] = 0;
				 
				 for (uint8_t rows = 0; rows < V[nib4]; rows++){
					uint8_t currByte = mem[i + rows];
					uint8_t getPixel = 0x80;
					for(int pixel = 0; pixel < 8; pixel++){
						uint16_t index = 64 * yPos + xPos;
						if(display[index] && display[index] ^ (getPixel & currByte))
							V[15] = 1;
						display[index] = display[index] ^ (getPixel & currByte);	

						if((index % X) == 63)
							break;
						xPos++;

					}
					yPos++;
					if (yPos == 31) 
						break;
 
				 }
				displayUpdated = 1;
				 
				break;

		}
		return displayUpdated;
}

	int main ( int argc, char* arg[]){
		
		if ( argc != 2 ){
			printf("Incorrect usage: %s, <rom file>" , argv[0]);
			exit(EXIT_FAILURE);
		} 
		
		load_rom(arg[1]);
		//rom now loaded into memory
		//set up SDL display
		if (SDL_Init(SDL_INIT_VIDEO)!= 0){

			printf("error initializing SD: %s\n", SDL_GetError());
			return 1;

		  }

		SDL_Window *win = SDL_CreateWindow("Hello!",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,640,480,0);
		


		  if(!win){

		    printf("error creating window%s\n",SDL_GetError() );
		    SDL_Quit();
		    return 1;

		  }

		unsigned int render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;

		SDL_Renderer *rend = SDL_CreateRenderer(win,-1,render_flags);
		SDL_RenderSetLogicalSize(rend, 64,32);

	  	if(!rend){
		    printf("error creating renderer: %s\n",SDL_GetError());
		    SDL_DestroyWindow(win);
		    SDL_Quit();
		    return 1;
		  }

		//fetch instructions 
		bool quit = 0;

		while(!quit){

			const uint64_t targetClockFrequency = 1.0/1000000.0;
			uint64_t elapsed;
			struct timespec start, end;

			clock_gettime(CLOCK_MONOTONIC, &start);

			bool displayUpdated = emulate_cycle();

			clock_gettime(CLOCK_MONOTONIC, &end);
			
			elapsed = (( end.tv_sec - start.tv_sec ) + ( end.tv_nsec - start.tv_nsec ) ) / 1e9;

			if(elapsed < instruction_time){
				struct timespec sleep_time;
				sleep_time.tv_sec = 0;
				sleep_time.tv_nsec = (instruction_time - elapsed) / 1e9;
				nanosleep(&sleep_time, NULL);
			}

			if(displayUpdated){

				SDL_SetRenderDrawColor(rend,0,0,0,1);
				SDL_RenderClear(rend);
				
				SDL_SetRenderDrawColor(rend,255,255,255,1);
				for (int i = 0; i < (64 * 32); i++){
					int pixelX = i % 64;
					int pixelY = i / 32;
					SDL_RenderDrawPoint(rend, pixelX, pixelY);
				}
				SDL_RenderPresent(rend);
			}



		}

		SDL_DestroyRenderer(rend);
		SDL_DestroyWindow(win);
		SDL_Quit();

		return 0;

	}

	
	


