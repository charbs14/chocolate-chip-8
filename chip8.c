#ifndef CHIP8_C
#define CHIP8_C

#include "chip8.h"
#include <iostream>
#include <stdint>
#include <unistd.h>
#include <stdio>

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

	uint8_t display[64][32];  //64 * 32 pixel display
					
					
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
	

	int main(int argc, char *argv[]){
			
		bool exit = 0;
		
		if(argc != 2){
			printf("Usage: %s <rom file>", argv[0]);
			exit(EXIT_FAILURE);
		}
		
		load_rom(argv[1]); //load rom into memory 


		while(!exit){
			//fetch instructions 
			uint16_t instruction = mem[pc] + mem[pc + 1]; //instruction is two bytes from memory 
			pc += 2;

			//decode instruction, first break into nibbles
			uint8_t nib1 = (instruction & 0xF000) >> 12;
			uint8_t nib2 = (instruction & 0x0F00) >> 8;
			uint8_t nib3 = (instruction & 0x00F0) >> 4;
			uint8_t nib4 = (instruction & 0x000F);

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
						
					break;

			}


		
			sleep(1/700);
		}


	}

