/*
    This is just a skeleton. It DOES NOT implement all the requirements.
    It only recognizes the RV32I "ADD", "SUB" and "ADDI" instructions only. 
    It prints "Unkown Instruction" for all other instructions!
    
    Usage example:
        $ rvcdiss t1.bin
    should print out:
        0x00000000  0x00100013  ADDI    x0, x0, 0x1
        0x00000004  0x00100093  ADDI    x1, x0, 0x1
        0x00000008  0x00100113  ADDI    x2, x0, 0x1
        0x0000000c  0x001001b3  ADD     x3, x0, x1
        0x00000010  0x00208233  ADD     x4, x1, x2
        0x00000014  0x004182b3  ADD     x5, x3, x4
        0x00000018  0x00100893  ADDI    x11, x0, 0x1
        0x0000001c  0x00028513  ADDI    xa, x5, 0x0
        0x00000020  0x00000073  Unkown Instruction 
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
// unordered_map <int, string> labels;

unsigned int pc = 0x0;

char memory[8*1024];    // only 8KB of memory located at address 0

// functions to print instruction

// takes only one register and unsigned immediate, bool sign determines whether signed or unsigned, bool address determines whether it is an address of a number
void printInstruction(unsigned int instWord, unsigned int instPC, string instruction, string rd, unsigned int immediate, bool sign, bool address = false)
{
    cout << "0x" << hex << setfill('0') << setw(8) << instPC << "\t0x" << setw(8) <<instWord << "\t" << dec;
    cout << instruction << " " << rd << ", ";
    if(address&&immediate!=0)
    {
        cout << "0x" << hex;
		cout << (int)immediate+instPC << "\n";
		return;
    }
    if(sign)
    {
        cout << (int) immediate << "\n";
    } 
    else
    {
        cout << immediate << "\n";
    }

}

// takes two registers and unsigned immediate, bool sign value determines whether it is signed or unsigned, address determines whether we should write hex format or decimal
void printInstruction(unsigned int instWord, unsigned int instPC, string instruction, string rd, string rs1, unsigned int immediate, bool sign, bool address  = false)
{
    cout << "0x" << hex << setfill('0') << setw(8) << instPC << "\t0x" << setw(8) <<instWord << "\t" << dec;
    cout << instruction << " " << rd << ", " << rs1 << ", ";
    if(address&&immediate!=0){
        cout << "0x" << hex;
		cout << (int)immediate+instPC << "\n";
		return;
    }
    if(sign)
    {
        cout << (int)immediate << "\n";
    } else
    {
        cout << immediate << "\n";
    }
}
// for unknown instructions
void printInstruction(unsigned int instWord, unsigned int instPC, bool unknown = false){
    cout << "0x" << hex << setfill('0') << setw(8) << instPC << "\t0x" << setw(8) <<instWord << "\t" << dec;
    if(unknown){
        cout << "Unknown Instruction Word\n";
        return; 
    }
}
// regular instructions with 3 registers
void printInstruction(unsigned int instWord, unsigned int instPC, string instruction, string rd, string rs1, string rs2){
    
    cout << "0x" << hex << setfill('0') << setw(8) << instPC << "\t0x" << setw(8) <<instWord << "\t" << dec;
    cout << instruction << " " << rd << ", " << rs1 << ", " << rs2 << "\n";
}

// store and load
void printInstruction(unsigned int instWord, unsigned int instPC, string instruction, string src, string base, unsigned int immediate = 0)
{
    cout << "0x" << hex << setfill('0') << setw(8) << instPC << "\t0x" << setw(8) <<instWord << "\t" << dec;

    cout << instruction << " " << src << ", " << (int)immediate << "(" << base << ")" << "\n";
}
void printInstruction_ecall(unsigned int instWord, unsigned int instPC, string instruction)
{
    cout << "0x" << hex << setfill('0') << setw(8) << instPC << "\t0x" << setw(8) <<instWord << "\t" << instruction << "\n";
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

    unsigned int instPC = pc - 4;

    opcode = instWord & 0x0000007F;
    rd = (instWord >> 7) & 0x0000001F;
    funct3 = (instWord >> 12) & 0x00000007;
    funct7 = (instWord >> 25) & 0x7F;
    rs1 = (instWord >> 15) & 0x0000001F;
    rs2 = (instWord >> 20) & 0x0000001F;

    //these are attained by looking at the RV32I instruction set and seeing where the immediate entries lie and how many shifts are necessary
    I_imm = ((instWord >> 20) & 0x7FF) | (((instWord >> 31) ? 0xFFFFF800 : 0x0));
	B_imm = ((instWord >> 7) & 0x1E) | ((instWord >> 20) & 0x7E0) | ((instWord << 4) & 0x800) | ((instWord >> 31) ? 0xFFFFF000 : 0x0);	
	S_imm = ((instWord >> 7) & 0x1F) | ((instWord >> 25) & 0x3F << 5) | (((instWord >> 31) ? 0xFFFFF800 : 0x0)); 
    U_imm = ((instWord & 0xFFFFF00) >> 12);
    J_imm = ((instWord & 0x7FE00000) >> 20) | ((instWord >> 20 & 0x1) << 11) | ((instWord >> 12 & 0xFF) << 12) | ((instWord >> 31) ? 0xFFFFF800 : 0x0);
/*for each instruction you will find  printinstruction function called based on the number of registers
The print function is overloaded so we pass the paramaters that fit and the rest is handeled
You can tell which instruction word it is from the string passed to the print function*/
    if(opcode == 0x3)
    {
        switch (funct3)
            {
            case 0: 
                printInstruction(instWord, instPC, "LB", reg32[rd], reg32[rs1], I_imm, true);
                break;
            case 1: 
                printInstruction(instWord, instPC, "LH", reg32[rd], reg32[rs1], I_imm, true);
                break;
            case 2: 
                printInstruction(instWord, instPC, "LW", reg32[rd], reg32[rs1], I_imm, true);
                break;
            case 4: 
                printInstruction(instWord, instPC, "LBU", reg32[rd], reg32[rs1], I_imm, false);
                break;
            case 5: 
                printInstruction(instWord, instPC, "LHU", reg32[rd], reg32[rs1], I_imm, false);
                break;
            case 6: 
                printInstruction(instWord, instPC, "LWU", reg32[rd], reg32[rs1], I_imm, false);
                break;
            default: 
                printInstruction(instWord, instPC, true);
                break;
            }
    }
    else if(opcode == 0x33){        // R Instructions
        switch(funct3){
            case 0:
                if (funct7 == 32) // Sub
                {
                    printInstruction(instWord, instPC, "SUB", reg32[rd], reg32[rs1], reg32[rs2]);
                }
                else // ADD
                {
                    printInstruction(instWord, instPC, "ADD", reg32[rd], reg32[rs1], reg32[rs2]);
                }
                break;

            case 1: // SLL
                printInstruction(instWord, instPC, "SLL", reg32[rd], reg32[rs1], reg32[rs2]);
                break;

            case 2: //SLT
                printInstruction(instWord, instPC, "SLT", reg32[rd], reg32[rs1], reg32[rs2]);
                break;

            case 3: //SLTU
                printInstruction(instWord, instPC, "SLTU", reg32[rd], reg32[rs1], reg32[rs2]); 
                break;

            case 4: //XOR
                printInstruction(instWord, instPC, "XOR", reg32[rd], reg32[rs1], reg32[rs2]);
                break;

            case 5:
                if (funct7 == 32) // SRA
                {
                    printInstruction(instWord, instPC, "SRA", reg32[rd], reg32[rs1], reg32[rs2]);
                }
                else // SRL
                {
                    printInstruction(instWord, instPC, "SRL", reg32[rd], reg32[rs1], reg32[rs2]);
                }
                break;

            case 6: //OR
                printInstruction(instWord, instPC, "OR", reg32[rd], reg32[rs1], reg32[rs2]);
                break;

            case 7: //AND
                printInstruction(instWord, instPC, "AND", reg32[rd], reg32[rs1], reg32[rs2]);
                break;

            default:
                printInstruction(instWord, instPC, true);
        }
    } 
    else if(opcode == 0x13) // I instructions -> missing JALR, LW, LH, LB
    {   
        switch(funct3){
            case 0:   // ADDI 
                printInstruction(instWord, instPC, "ADDI", reg32[rd], reg32[rs1], I_imm, true);
                break;

            case 1: //SLLI
                printInstruction(instWord, instPC, "SLLI", reg32[rd], reg32[rs1], I_imm, true);
                break;

            case 2: //SLTI
                printInstruction(instWord, instPC, "SLTI", reg32[rd], reg32[rs1], I_imm, true);
                break;

            case 3: //SLTIU
                printInstruction(instWord, instPC, "SLTIU", reg32[rd], reg32[rs1], I_imm, false);
                break;

            case 4: //XORI 
                printInstruction(instWord, instPC, "XORI", reg32[rd], reg32[rs1], I_imm, true);
                break;

            case 5: //SRLI & SRAI
                if (funct7 == 0)
                {
                    printInstruction(instWord, instPC, "SRLI", reg32[rd], reg32[rs1], I_imm, true);

                }
                else 
                {
                    printInstruction(instWord, instPC, "SRAI", reg32[rd], reg32[rs1], I_imm, true);
                }
                break;

            case 6: //ORI
                printInstruction(instWord, instPC, "ORI", reg32[rd], reg32[rs1], I_imm, true);
                break;

            case 7: //ANDI
                printInstruction(instWord, instPC, "ANDI", reg32[rd], reg32[rs1], I_imm, true);
                break;

            default:
                printInstruction(instWord, instPC, true);
                break;
        }
    }
    else if(opcode==0x63)   //B instructions
    { 
        switch(funct3){
            case 0:// BEQ
                printInstruction(instWord, instPC, "BEQ", reg32[rs1], reg32[rs2], B_imm, true, true);
                break;

            case 1: //BNE
                printInstruction(instWord, instPC, "BNE", reg32[rs1], reg32[rs2], B_imm, true, true);
                break;

            case 4: //BLT
                printInstruction(instWord, instPC, "BLT", reg32[rs1], reg32[rs2], B_imm, true, true);
                break;

            case 5: //BGE
                printInstruction(instWord, instPC, "BGE", reg32[rs1], reg32[rs2], B_imm, true, true);
                break;

            case 6://BLTU
                printInstruction(instWord, instPC, "BLTU", reg32[rs1], reg32[rs2], B_imm, false, true);
                break;

            case 7: //BGEU
                printInstruction(instWord, instPC, "BGEU", reg32[rs1], reg32[rs2], B_imm, false, true);
                break;

            default:
                printInstruction(instWord, instPC, true);
                break;

        }
    }       
    else if (opcode == 0x37) // LUI-U instruction
    {
        printInstruction(instWord, instPC, "LUI", reg32[rd], U_imm, true);
    }
    else if (opcode == 0x17) //AUIPC- U instruction
    {
        printInstruction(instWord, instPC, "AUIPC", reg32[rd], U_imm, true);
    }
    else if (opcode == 0x23) // S Instructions
    {
        switch (funct3)
            {
            case 0:  //SB
                printInstruction(instWord, instPC, "SB", reg32[rs2], reg32[rs1], S_imm);
                break;

            case 1:  //SH
                printInstruction(instWord, instPC, "SH", reg32[rs2], reg32[rs1], S_imm);
                break;

            case 2:  //SW
                printInstruction(instWord, instPC, "SW", reg32[rs2], reg32[rs1], S_imm);
                break;

            default:
                printInstruction(instWord, instPC, true);
                break;
            }
    }
    else if(opcode == 0x67) // I Type, JALR
    {
        printInstruction(instWord, instPC, "JALR", reg32[rd], reg32[rs1], I_imm, true, true);
    }
    else if(opcode == 0x6F) //J-TYPE
    {
        printInstruction(instWord, instPC, "JAL", reg32[rd], J_imm, true, true);
    }
    else if (opcode == 0x73)
    {
        printInstruction_ecall(instWord, instPC, "ecall");
    }
    else 
    {
        printInstruction(instWord, instPC, true);
    }
} 

// for rv32ic instructions
void instDecExec(unsigned int instWord)
{
    unsigned int address;
    // pc -2 as it is 16 bits
    unsigned int instructionPC = pc-2; 
    unsigned int opcode = instWord & 0x3; // and with 00..11 to get opcode
    unsigned int funct3 = (instWord >> 13) & 0x7; // funct3 in same location for all
    unsigned int funct7 = (instWord >> 12) & 0x1;
    unsigned int functExt = (instWord >> 5) & 0x3;
    // registers
    unsigned int rd32, rs32, rs32_2;
    rd32 = rs32 = (instWord >> 7) & 0x1F;
    rs32_2 = (instWord >> 2) & 0x1F;

    unsigned int rd16, rs16, rs16_2;;
    rd16=rs16 = (instWord >> 7) & 0x7;
    rs16_2 = (instWord >> 2) & 0x7;

    //imm
    unsigned int J_imm = (((instWord >> 3) & 0x7) << 1) | (((instWord >> 11) & 0x1) << 4) | (((instWord >> 2) & 0x1) << 5) | (((instWord >> 7) & 0x1) << 6) | (((instWord >> 6) & 0x1) << 7) | (((instWord >> 9) & 0x3) << 8) | (((instWord >> 8) & 0x1) << 10) | ((((instWord >> 12)&0x1) ? 0xFFFFF800: 0x0));
    unsigned int I_imm = (((instWord >> 12)& 0x1) ? 0xFFFFFFE0: 0x0) | (instWord >> 2) & 0x1F;
    unsigned int immLS = (((instWord >> 6) & 0x1) << 2)| (((instWord >> 5) & 0x1) << 6) | ((( instWord >> 10) & 0x7) << 3); // 0 extended
    unsigned int B_imm = (((instWord >> 12) & 0x1) ? 0xFFFFFF00: 0x0) | (((instWord >> 3) & 0x3) << 1) | (((instWord >> 10) & 0x3) << 3) | (((instWord >> 2) & 0x1) << 5) | (((instWord > 5) & 0x3) << 6);
	if(opcode == 0x0) // load/store 
    {   
        unsigned int lsrd16 = (instWord >> 2) & 0x7; // only supports compressed registers for L/S / base or dest
        unsigned int lsrs1_16 = (instWord >> 7) & 0x7; //base
        switch(funct3){
            case 2: // C.LW
            {
                printInstruction(instWord, instructionPC, "C.LW", reg16[lsrd16], reg16[lsrs1_16], immLS);
                break;
            }
            case 6: // C.SW
            {
                printInstruction(instWord, instructionPC, "C.SW", reg16[lsrd16], reg16[lsrs1_16], immLS);
                break;
            }
            default:
                    printInstruction(instWord, instructionPC, true);
                    break;

        }
    }
    else if (opcode == 0x1)
    {
        switch (funct3){
            case 0: // either nop or addi
                {
                    int imm_5 =  (instWord >> 12) & 0x1; // check if bit at index 12 is 0 or not
                    if(imm_5) // if 1 -> addi -> supports 32 bit registers -> signed imm
                    {
                        printInstruction(instWord, instructionPC, "C.ADDI", reg32[rd32], reg32[rs32], I_imm, true);
                    }
                    else //NOP -> pseudo so we convert to true
                    {
                        printInstruction(instWord, instructionPC, "C.ADDI", reg32[0], reg32[0], 0, true);
                    }
                    
                }
                break;
            case 1: // JAL -> signed imm
                    printInstruction(instWord, instructionPC, "C.JAL", reg32[1], J_imm, true, true);
                    break;

            case 2: // LI -> pseudo so we convert to true addi rd, rd, imm -> signed imm
                    printInstruction(instWord, instructionPC, "C.ADDI", reg32[rd32], reg32[0], I_imm, true);
                    break;

            case 3: // LUI -> signed imm
                {
                    unsigned int imm17 = (instWord >> 12) & 0x1;
                    unsigned int imm12to16 = (instWord >> 2) & 0x1F;
                    unsigned int imm = ((((imm17) ? 0xFFFFFFE0: 0x0) | imm12to16) << 12);

                    printInstruction(instWord, instructionPC, "C.LUI", reg32[rd32], imm, true);
                    
                }
                break;

            case 4: // many options
                {
                    switch(funct7)
                        {
                            case 0: //SRLI 
                                    printInstruction(instWord, instructionPC, "C.SRLI", reg16[rd16], reg16[rs16], I_imm, true);
                                    break;
                            case 1: //SRAI 
                                    printInstruction(instWord, instructionPC, "C.SRAI", reg16[rd16], reg16[rs16], I_imm, true);
                                    break;
                            case 2: // ANDI 
                                    printInstruction(instWord, instructionPC, "C.ANDI", reg16[rd16], reg16[rs16], I_imm, true);
                                    break;
                            case 3: // more options
                                {
                                    switch(functExt)
                                        {
                                            case 0: // SUB
                                                    printInstruction(instWord, instructionPC, "C.SUB", reg16[rd16], reg16[rs16], reg16[rs16_2]);
                                                    break;
                                            case 1: // XOR
                                                    printInstruction(instWord, instructionPC, "C.XOR", reg16[rd16], reg16[rs16], reg16[rs16_2]);
                                                    break;
                                            case 2: // OR
                                                    printInstruction(instWord, instructionPC, "C.OR", reg16[rd16], reg16[rs16], reg16[rs16_2]);
                                                    break;
                                            case 3: // AND
                                                    printInstruction(instWord, instructionPC, "C.AND", reg16[rd16], reg16[rs16], reg16[rs16_2]);
                                                    break;
                                            default:
                                                    printInstruction(instWord, instructionPC, true);
                                                    break;
                                        }
                                }
                            default:
                                    printInstruction(instWord, instructionPC, true);
                                    break;
                        } 
                }
                break;
            case 5: //J -> convert to JAL
                    printInstruction(instWord, instructionPC, "C.JAL", reg32[0], J_imm, true, true);
                    break;
            case 6: //BEQZ pseudo so we translate to beq w/r to zero register, signed 
                    printInstruction(instWord, instructionPC, "C.BEQ", reg16[rs16], reg16[0], B_imm, true, true);
                    break;
            case 7: //BNEZ 
                    printInstruction(instWord, instructionPC, "C.BNE", reg16[rs16], reg16[0], B_imm, true, true);
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
                printInstruction(instWord, instructionPC, "C.SLLI", reg32[rd32], reg32[rs32], I_imm);
                break;

            case 4: 
                {
                    switch(funct7)
                    {
                        case 0:
                            if(rs32_2 == 0x0) // JR
                            {
                                printInstruction(instWord, instructionPC, "C.JALR", reg32[0],  reg32[rs32], 0, true, true);
                            }
                            else if(rs32_2 != 0x0) // MV -> switch to true instruction 
                            {
                                printInstruction(instWord, instructionPC, "C.ADDI", reg32[rd32], reg32[rs32_2], 0, true);
                            }
                            else // unknown 
                            {
                                printInstruction(instWord, instructionPC, true);
                            }
                            break;
                        case 1:
                            if(rs32_2 == 0x0) // JALR -> jalr x1, rs1, 0
                            {
                                printInstruction(instWord, instructionPC, "C.JALR", reg32[1],  reg32[rs32], 0, true, true);
                            }
                            else if(rs32_2 != 0x0) // ADD 
                            {
                                printInstruction(instWord, instructionPC, "C.ADD", reg32[rd32], reg32[rs32], reg32[rs32_2]);
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
                break;
				case 6: // C.SWSP
						{
								unsigned int imm = (((instWord >> 7) & 0x3) << 7) | (((instWord >> 9) & 0xF) << 2);
								printInstruction(instWord, instructionPC, "C.SWSP", reg32[rs32_2], reg32[2], imm);
							}
							break;
            default:
                printInstruction(instWord, instructionPC, true);    
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
            int temp = memory[pc] & 0x00000003; //anding with 3 to check whether it is 32 or 16 bit. 
            bool is32 = false;

            if(temp==3){ //if the anded result gives us 3 then it is 32 bits
                instWord =  (unsigned char)memory[pc] |
                            (((unsigned char)memory[pc+1])<<8) |
                            (((unsigned char)memory[pc+2])<<16) |
                            (((unsigned char)memory[pc+3])<<24);
                pc += 4;
                is32 = true;
                instDecExec(instWord, is32);
            }
            else {
                instWord = (unsigned char)memory[pc] | (((unsigned char)memory[pc + 1]) << 8);
                is32 = false;
                pc += 2;
                instDecExec(instWord);
            }
                // remove the following line once you have a complete simulator
                
                if (!memory[pc]) break; //stop when we've gone through all the instructions in the memory.
        }

    } else emitError("Cannot access input file\n");
}

