/*
	This is just a skeleton. It DOES NOT implement all the requirements.
	It only recognizes the RV32I "ADD", "SUB" and "ADDI" instructions only. 
	It prints "Unkown Instruction" for all other instructions!
	
	Usage example:
		$ rvcdiss t1.bin
	should print out:
		0x00000000	0x00100013	ADDI	x0, x0, 0x1
		0x00000004	0x00100093	ADDI	x1, x0, 0x1
		0x00000008	0x00100113	ADDI	x2, x0, 0x1
		0x0000000c	0x001001b3	ADD		x3, x0, x1
		0x00000010	0x00208233	ADD		x4, x1, x2
		0x00000014	0x004182b3	ADD		x5, x3, x4
		0x00000018	0x00100893	ADDI	x11, x0, 0x1
		0x0000001c	0x00028513	ADDI	xa, x5, 0x0
		0x00000020	0x00000073	Unkown Instruction 
	middle column is the 32 bit number representing the I/J/U/R binary format of the assembly line
	References:
	(1) The risc-v ISA Manual ver. 2.1 @ https://riscv.org/specifications/
	(2) https://github.com/michaeljclark/riscv-meta/blob/master/meta/opcodes

	 1) Find out if its 16 or 32 bits
 	2) create S,B,J,U instructions
 	3) complete R, I instructions
 	4) BU: salma
	SJ: Allaa
	IR: MO
*/

#include <iostream>
#include <fstream>
#include "stdlib.h"
#include <iomanip>
#include <unordered_map>
#define MAX_LABEL_SIZE 1000
using namespace std;

// registers for 32-bit instructions
string reg32[32] = {"zero", "ra", "sp", "gp",
					"tp","t0", "t1", "t2", 
					"s0", "s1","a0", "a1", 
					"a2", "a3", "a4", "a5",
					"a6", "a7","s2", "s3",
					"s4", "s5", "s6", "s7",
					"s8", "s9", "s10", "s11",
					"t3", "t4", "t5", "t6"};
					
// registers for compressed 16-bit instructions
string reg16[8] = {"s0", "s1", "a0", "a1",
 				   "a2", "a3", "a4", "a5"};

// label table
unordered_map <int, string> labels;

unsigned int pc = 0x0;

char memory[8*1024];	// only 8KB of memory located at address 0

// functions to print instruction
// 
// takes only one register and unsigned immediate
void printInstruction(unsigned int instWord, unsigned int instPC, string instruction, string rd, unsigned int immediate)
{
	cout << "0x" << hex << setfill('0') << setw(8) << instPC << "\t0x" << setw(8) <<instWord << "\t" << dec;
	cout << instruction << " " << rd << ", " << immediate << "\n";

}
// takes two registers and unsigned immediate
void printInstruction(unsigned int instWord, unsigned int instPC, string instruction, string rd, string rs1, unsigned int immediate)
{
	cout << "0x" << hex << setfill('0') << setw(8) << instPC << "\t0x" << setw(8) <<instWord << "\t" << dec;
	cout << instruction << " " << rd << ", " << rs1 << ", "<< immediate << "\n";

}

// for unknowned instructions
void printInstruction(unsigned int instWord, unsigned int instPC, bool unknown = false){
	cout << "0x" << hex << setfill('0') << setw(8) << instPC << "\t0x" << setw(8) <<instWord << "\t" << dec;
	if(unknown){
		cout << "Unknown Instruction Word\n";
		return; 
	}
}
// regular instructions with 3 registers
void printInstruction(unsigned int instWord, unsigned int instPC, string instruction, string rd, string rs1, string rs2, unsigned int immediate = 0, bool address = false, bool memoryAccess = false){
	
	cout << "0x" << hex << setfill('0') << setw(8) << instPC << "\t0x" << setw(8) <<instWord << "\t" << dec;
	// if unknown print unknown and exit
	
	// if not imm and address and memory access -> function has 3 registers
	if(!immediate && !address && !memoryAccess)
	{
		 cout << instruction << " " << rd << ", " << rs1 << ", " << rs2 << "\n";
	}else{
		// default part of output for all other instructions
		cout << instruction << " " << rd << ", ";
		// check if not memory access and rs1 != ""
		if(!memoryAccess && rs1 != "")
		{
			cout << rs1 << ", ";
		}

		// check if label
		if(address)
		{
			cout << hex << "0x";
		}
		
		cout << (int)immediate;
		
		// check for address
		if(address)
		{
			// to be implemented
		}

		// check if we are accessing memory in instruction
		if(memoryAccess)
		{
			cout << "(" << rs1 << ")";
		}

		cout << "\n";
	}
}

// for instructions with signed immediate
void printInstruction(unsigned int instWord, unsigned int instPC, string instruction, string rd, string rs1, int immediate = 0, bool address = false, bool memoryAccess = false)
{
	cout << "0x" << hex << setfill('0') << setw(8) << instPC << "\t0x" << setw(8) <<instWord << "\t" << dec;
	// if unknown print unknown and exit

	// if not imm and address and memory access -> function has 3 registers

	// default part of output for all other instructions
	cout << instruction << " " << rd << ", ";
	// check if not memory access and rs1 != ""
	if(!memoryAccess && rs1 != "")
	{
		cout << rs1 << ", ";
	}

	// check if label
	if(address)
	{
		cout << hex << "0x";
	}
	
	cout << (int)immediate;
	
	// check for address
	if(address)
	{
		// to be implemented
	}

	// check if we are accessing memory in instruction
	if(memoryAccess)
	{
		cout << "(" << rs1 << ")";
	}

	cout << "\n";
}



void emitError(char *s)
{
	cout << s;
	exit(0);
}

// for risc32i instructions
void instDecExec(unsigned int instWord, bool is32)
{
	unsigned int rd, rs1, rs2, funct3, funct7, opcode; //for 32 bits
	unsigned int I_imm, S_imm, B_imm, U_imm, J_imm; //for the immediate values of the 32 bits
	unsigned int address;

	if(is32){
	unsigned int instPC = pc - 4;

	opcode = instWord & 0x0000007F;
	rd = (instWord >> 7) & 0x0000001F;
	funct3 = (instWord >> 12) & 0x00000007;
	rs1 = (instWord >> 15) & 0x0000001F;
	rs2 = (instWord >> 20) & 0x0000001F;

    //these are attained by looking at the RV32I instruction set and seeing where the immediate entries lie and how many shifts are necessary
	I_imm = ((instWord >> 20) & 0x7FF) | (((instWord >> 31) ? 0xFFFFF800 : 0x0));
	B_imm = ((instWord >> 7 & 0x1) << 12) | ((instWord >> 25 & 0x3F) << 5) | (instWord >> 8 & 0xF) | ((instWord >> 31) ? 0xFFFFF800 : 0x0);
    // imm[11]                           imm[10:5]                                       imm[4:1]
	U_imm = ((instWord & 0xFFFFF00) >> 12);

/*for each instruction you will find  printinstruction function called based on the number of registers
The print function is overloaded so we pass the paramaters that fit and the rest is handeled
You can tell which instruction word it is from the string passed to the print function*/

	if(opcode == 0x33){		// R Instructions
		switch(funct3){
			case 0:{
				if (funct7 == 32)
					printInstruction(instWord, instPC, "SUB", reg32[rd], reg32[rs1], reg32[rs2]);
				else
                    printInstruction(instWord, instPC, "ADD", reg32[rd], reg32[rs1], reg32[rs2]);

				break;}
			case 1:
                printInstruction(instWord, instPC, "SLL", reg32[rd], reg32[rs1], reg32[rs2]);
				break;
			case 2:
                printInstruction(instWord, instPC, "SLT", reg32[rd], reg32[rs1], reg32[rs2]);
				break;
			case 3:
                printInstruction(instWord, instPC, "SLTU", reg32[rd], reg32[rs1], reg32[rs2]); //not sure about this cuz of unsigned
				break;
			case 4:
                printInstruction(instWord, instPC, "XOR", reg32[rd], reg32[rs1], reg32[rs2]);
				break;
			case 5:
				if (funct7 == 32)
                    printInstruction(instWord, instPC, "SRA", reg32[rd], reg32[rs1], reg32[rs2]);
				else
                    printInstruction(instWord, instPC, "SRL", reg32[rd], reg32[rs1], reg32[rs2]);

				break;
			case 6:
                printInstruction(instWord, instPC, "OR", reg32[rd], reg32[rs1], reg32[rs2]);
				break;

			case 7:
                printInstruction(instWord, instPC, "AND", reg32[rd], reg32[rs1], reg32[rs2]);
				break;
			default:
				cout << "\tUnknown R Instruction \n";
		}
	} else if(opcode == 0x13){	// I instructions
		switch(funct3){
			case 0:	cout << "\tADDI\tx" << rd << ", x" << rs1 << ", " << hex << "0x" << (int)I_imm << "\n";
					break;
			default:
					cout << "\tUnkown I Instruction \n";
		}
	} else if(opcode==0x63){ //B instructions
		switch(funct3){
			case 0:
                printInstruction(instWord, instPC, "BEQ", reg32[rs1], reg32[rs2], (int)B_imm);

				break;
			case 1:
                printInstruction(instWord, instPC, "BNE", reg32[rs1], reg32[rs2], (int)B_imm);
				break;
			case 4:
                printInstruction(instWord, instPC, "BLT", reg32[rs1], reg32[rs2], (int)B_imm);
				break;
			case 5:
                printInstruction(instWord, instPC, "BGE", reg32[rs1], reg32[rs2], (int)B_imm);
				break;
			case 6:
                printInstruction(instWord, instPC, "BLTU", reg32[rs1], reg32[rs2], B_imm);
				break;
			case 7:
                printInstruction(instWord, instPC, "BGEU", reg32[rs1], reg32[rs2], B_imm);
				break;
			default:
				cout << " Unkown B Instruction " << "\n";
		}
	} 		else if (opcode == 0x37) // LUI-U instruction
		{
			cout << "\tLUI\t" << reg32[rd] << ", " << hex << "0x" << (int)U_imm << endl; //one register and a signed immediate
		}

		else if (opcode == 0x17) //AUIPC- U instruction
		{
			cout << "\tAUIPC\t" << reg32[rd] << ", " << hex << "0x" << (int)U_imm << endl; //one register and a signed immediate
		}
	else {
		cout << "\tUnkown Instruction \n";
	}

	} else{ //16 bit instructions

	}
}

// for rv32ic instructions
void instDecExec(unsigned int instWord)
{
	unsigned int rd, rs1, rs2, funct3, funct7, opcode;
	unsigned int I_imm, S_imm, B_imm, U_imm, J_imm;
	unsigned int address;

	unsigned int instPC = pc - 4;
	// treat 16 and 32 bit instruction differently
		// pc -2 as it is 16 bits
	unsigned int instructionPC = pc-2; 
	unsigned int opcode = instWord & 0x0003; // and with 00..11 to get opcode
	unsigned int funct3 = (instWord >> 13) & 0x7; // funct3 in same location for all
	// check opcode
	if(opcode == 0x0) // load/store 
	{	
		// get individual imm at each index
		unsigned int imm2 = (instWord >> 6) & 0x0001;
		unsigned int imm6 = (instWord >> 5) & 0x0001;
		unsigned int imm3to5 = ( instWord >> 10) & 0x0007;
		// combine and scale by 4
		unsigned int immLS = ((imm2 & 0x1) << 2)| ((imm6 & 0x1) << 6) | ((imm3to5 & 0x7) << 3);
		
		

		unsigned int rd16 = (instWord >> 2) & 0x0007; // only supports compressed registers for L/S
		unsigned int rs1_16 = (instWord >> 7) & 0x0007; 
		switch(funct3){
			case 2: // C.LW
					printInstruction(instWord, instructionPC, "C.LW", reg16[rd16], reg16[rs1_16], "", immLS, false, true);
					break;
			case 6: // C.SW
					printInstruction(instWord, instructionPC, "C.SW", reg16[rd16], reg16[rs1], "", immLS, false, true);
					break;
			default:
					printInstruction(instWord, instructionPC, true);
					break;

		}
	}
	else if (opcode == 0x1)
	{
		switch (funct3){
			case 0: // either nop or addi
					int imm_5 = (instWord >> 12) & 0x1; // check if bit at index 12 is 0 or not
					if(imm_5) // if 1 -> addi -> supports 32 bit registers -> signed imm
					{
						unsigned int rd32, rs32;
						rd32 = rs32 = (instWord >> 7) & 0b11111; // rd and rs1 are the same
						
						int imm0to4 = (instWord >> 2) & 0x1F;
						int imm = ((imm_5) ? 0xFFFFFFE0: 0x0) | imm0to4;

						printInstruction(instWord, instructionPC, "C.ADDI", reg32[rd32], reg32[rs32], "", imm);
					}
					else //NOP -> pseudo so we convert to true
					{
						printInstruction(instWord, instructionPC, "C.ADDI", reg32[0], reg32[0], "", 0);
					}
					break;
			case 1: // JAL -> signed imm
					int imm5 = (instWord >> 2) & 0x1;
					int imm1to3 = (instWord >> 3) & 0x7;
					int imm7 = (instWord >> 6) & 0x1;
					int imm6 = (instWord >> 7) & 0x1;
					int imm10 = (instWord >> 8) & 0x1;
					int imm8to9 = (instWord >> 9) & 0x3;
					int imm4 = (instWord >> 11) & 0x1;
					int imm11 = (instWord >> 12) & 0x1;
					int imm = ((imm1to3 & 0x7) << 1) | ((imm4 & 0x1) << 4) | ((imm5 & 0x1) << 5) | ((imm6 & 0x1) << 6) | ((imm7 & 0x1) << 7) | ((imm8to9 & 0x3) << 8) | ((imm10 & 0x1) << 10) | ((imm11 ? 0xFFFFF800: 0x0)) ;
					printInstruction(instWord, instructionPC, "C.JAL", reg32[1], imm);
					break;
			case 2: // LI -> pseudo so we convert to true addi rd, rd, imm -> signed imm
					unsigned int rd32, rs32;
					rd32 = rs32 = (instWord >> 7) & 0x1F;

					int imm5 = (instWord >> 12) & 0x1;
					int imm0to4 = (instWord >> 2) & 0x1F;
					int imm = ((imm5) ? 0xFFFFFFE0: 0x0) | imm0to4;

					printInstruction(instWord, instructionPC, "C.ADDI", reg32[rd32], reg32[rs32], "", imm);
					break;
			case 3:	// LUI -> signed imm
					unsigned int rd32 = (instWord >> 7) & 0x1F;
					int imm17 = (instWord >> 12) & 0x1;
					int imm12to16 = (instWord >> 2) & 0x1F;
					int imm = ((((imm17) ? 0xFFFFFFE0: 0x0) | imm12to16) << 12);
					printInstruction(instWord, instructionPC, "C.LUI", reg32[rd32], imm);
					break;
			case 4: // many options
					unsigned int funct7 = (instWord >> 10) & 0x2; // will use to differentiate between instructions further
					switch(funct7)
					{
						case 0: //SRLI -> unsigned imm -> supports 16 bit registers only
								unsigned int rd16, rs16;
								rd16=rs16 = (instWord >> 7) & 0x7;
								unsigned int imm5 = (instWord >> 12) & 0x1;
								unsigned int imm0to4 = (instWord >> 2) & 0x1F;
								unsigned int imm = ((imm5 & 0x1) << 5) | (imm0to4 & 0x1F);
								printInstruction(instWord, instructionPC, "C.SRLI", reg16[rd16], reg16[rs16], imm);
								break;
						case 1: //SRAI -> unsigned immediate
								unsigned int rd16, rs16;
								rd16=rs16 = (instWord >> 7) & 0x7;
								unsigned int imm5 = (instWord >> 12) & 0x1;
								unsigned int imm0to4 = (instWord >> 2) & 0x1F;
								unsigned int imm = ((imm5 & 0x1) << 5) | (imm0to4 & 0x1F);
								printInstruction(instWord, instructionPC, "C.SRAI", reg16[rd16], reg16[rs16], imm);
								break;
						case 2: // ANDI -> signed immediate
								unsigned int rd16, rs16;
								rd16=rs16 = (instWord >> 7) & 0x7;
								int imm_5 = (instWord >> 12) & 0x1;
								int imm0to4 = (instWord >> 2) & 0x1F;
								int imm = ((imm_5) ? 0xFFFFFFE0: 0x0) | imm0to4;
								printInstruction(instWord, instructionPC, "C.ANDI", reg16[rd16], reg16[rs16], imm);
								break;
						case 3: // more options
								unsigned int functExt = (instWord >> 5) & 0x3;
								unsigned int rd16, rs16_1, rs16_2;
								rd16=rs16_1 = (instWord >> 7) & 0x7;
								rs16_2 = (instWord >> 2) & 0x7;

								switch(functExt)
							
								{
									case 0: // SUB
											printInstruction(instWord, instructionPC, "C.SUB", reg16[rd16], reg16[rs16_1], reg16[rs16_2]);

											break;
									case 1: // XOR
											printInstruction(instWord, instructionPC, "C.XOR", reg16[rd16], reg16[rs16_1], reg16[rs16_2]);

											break;
									case 2: // OR
											printInstruction(instWord, instructionPC, "C.OR", reg16[rd16], reg16[rs16_1], reg16[rs16_2]);

											break;
									case 3: // AND
											printInstruction(instWord, instructionPC, "C.AND", reg16[rd16], reg16[rs16_1], reg16[rs16_2]);
											break;
									default:
											printInstruction(instWord, instructionPC, true);
											break;
								}
						default:
								printInstruction(instWord, instructionPC, true);
								break;
					} 
					break;
			case 5: //J
					int imm5 = (instWord >> 2) & 0x1;
					int imm1to3 = (instWord >> 3) & 0x7;
					int imm7 = (instWord >> 6) & 0x1;
					int imm6 = (instWord >> 7) & 0x1;
					int imm10 = (instWord >> 8) & 0x1;
					int imm8to9 = (instWord >> 9) & 0x3;
					int imm4 = (instWord >> 11) & 0x1;
					int imm11 = (instWord >> 12) & 0x1;
					int imm = ((imm1to3 & 0x7) << 1) | ((imm4 & 0x1) << 4) | ((imm5 & 0x1) << 5) | ((imm6 & 0x1) << 6) | ((imm7 & 0x1) << 7) | ((imm8to9 & 0x3) << 8) | ((imm10 & 0x1) << 10) | ((imm11 ? 0xFFFFF800: 0x0)) ;
					printInstruction(instWord, instructionPC, "C.JAL", reg32[0], imm);
					break;
			case 6: //BEQZ pseudo so we translate to beq w/r to zero register, signed 
					unsigned int rs16 = (instWord >> 7) & 0x7; // supports the compressed registers only
					int imm8 = (instWord >> 12) & 0x1;
					int imm3to4 = (instWord >> 10) & 0x3;
					int imm5 = (instWord >> 2) & 0x1;
					int imm1to2 = (instWord >> 3) & 0x3;
					int imm6to7 = (instWord > 5) & 0x3;
					int imm = (imm8 ? 0xFFFFFF00: 0x0) | ((imm1to2& 0x3) << 1) | ((imm3to4 & 0x3) << 3) | ((imm5 & 0x1) << 5) | ((imm6to7 & 0x3) << 6);
					printInstruction(instWord, instructionPC, "C.BEQ", reg16[rs16], reg16[0], imm);
					break;
			case 7: //BNEZ 
					unsigned int rs16 = (instWord >> 7) & 0x7; // supports the compressed registers only
					int imm8 = (instWord >> 12) & 0x1;
					int imm3to4 = (instWord >> 10) & 0x3;
					int imm5 = (instWord >> 2) & 0x1;
					int imm1to2 = (instWord >> 3) & 0x3;
					int imm6to7 = (instWord > 5) & 0x3;
					int imm = (imm8 ? 0xFFFFFF00: 0x0) | ((imm1to2& 0x3) << 1) | ((imm3to4 & 0x3) << 3) | ((imm5 & 0x1) << 5) | ((imm6to7 & 0x3) << 6);
					printInstruction(instWord, instructionPC, "C.BNE", reg16[rs16], reg16[0], imm);
					break;
			default:
					printInstruction(instWord, instructionPC, true);
					break;
		}
	}
	else if(opcode == 0x2)
	{
		switch(funct3)
		{
			case 0: // slli 
				unsigned int rd16, rs16;
				rd16=rs16 = (instWord >> 7) & 0x1F;
				unsigned int imm5 = (instWord >> 12) & 0x1;
				unsigned int imm0to4 = (instWord >> 2) & 0x1F;
				unsigned int imm = ((imm5 & 0x1) << 5) | (imm0to4 & 0x1F);
				printInstruction(instWord, instructionPC, "C.SRLI", reg16[rd16], reg16[rs16], imm);
				break;
			case 4:
				unsigned int funct7 = (instWord >> 12) & 0x1;
				unsigned int rs32_1, rd32, rs32_2;
				rs32_1 = rd32 = (instWord >> 7) & 0x1F;
				rs32_2 = (instWord >> 2) & 0x1F;
				switch(funct7)
				{
					case 0:
						if(rs32_2 == 0x0) // JR
						{
							printInstruction(instWord, instructionPC, "C.JALR", reg32[0],  reg32[rs32_1], 0);
						}
						else if(rs32_2 != 0x0) // MV -> switch to true instruction
						{
							printInstruction(instWord, instructionPC, "C.ADDI", reg32[rd32], reg32[rs32_2], 0);
						}
						else // unknown 
						{
							printInstruction(instWord, instructionPC, true);
						}
						break;
					case 1:
						if(rs32_2 == 0x0) // JALR -> jalr x1, rs1, 0
						{
							printInstruction(instWord, instructionPC, "C.JALR", reg32[1],  reg32[rs32_1], 0);
						}
						else if(rs32_2 != 0x0) // ADD 
						{
							printInstruction(instWord, instructionPC, "C.ADD", reg32[rd32], reg32[rs32_1], reg32[rs32_2]);
						}
						else // unknown 
						{
							printInstruction(instWord, instructionPC, true);
						}
						break;
					default:
							printInstruction(instWord, instructionPC, true);
							break;
				}	
		}
	}
	else
	{
		printInstruction(instWord, instructionPC, true);
	}
}


int main(int argc, char *argv[]){

	unsigned int instWord=0;
	ifstream inFile;
	ofstream outFile;

	if(argc<2) emitError("use: rvcdiss <machine_code_file_name>\n");
	
	inFile.open(argv[2], ios::in | ios::binary | ios::ate);
	
	if(inFile.is_open())
	{
		int fsize = inFile.tellg();

		inFile.seekg (0, inFile.beg);
		if(!inFile.read((char *)memory, fsize)) emitError("Cannot read from input file\n");

		while(true){
			int temp = memory[pc] & 0x00000003;	//anding with 3 to check whether it is 32 or 16 bit. 
			bool is32 = true;

			if(is32==3){ //if the anded result gives us 3 then it is 32 bits
				instWord = 	(unsigned char)memory[pc] |
							(((unsigned char)memory[pc+1])<<8) |
							(((unsigned char)memory[pc+2])<<16) |
							(((unsigned char)memory[pc+3])<<24);
				pc += 4;
			}
			else {
				instWord = (unsigned char)memory[pc] | (((unsigned char)memory[pc + 1]) << 8);
				is32 = false;
				pc += 2;
			}
				// remove the following line once you have a complete simulator
				instDecExec(instWord, is32);
				if (!memory[pc]) break;	//stop when we've gone through all the instructions in the memory.
		}

	} else emitError("Cannot access input file\n");
}
