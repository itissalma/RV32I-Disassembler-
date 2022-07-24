# rv32ic-disassembler
# Implementation details:
The program starts by accepting machine code that is read from a binary file and stores it in an array of 8kb called memory. It calls the emitError function if the arguments are less than 2. We read instruction by instruction until we've gone through all the instructions in the memory. We then anded the memory address of each instruction with 3 to check whether it's a 32 or 16-bit instruction and enter different functions accordingly. We need to do this because this disassembler supports RV32IC instructions, which means that compressed instructions could be mixed with 32-bit instructions. If the instruction is 32-bits, we take 32 bits from the binary file by concatenating four 8-bits from the memory file; then the program counter is incremented by 4. Otherwise, if the two least significant bits are not one, we concatenate only 2 bytes from memory using the program counter location. The program counter is incremented by two in this case.

The logic of both the 32bit and 16bit functions which are overloaded into instDecExec, are the same; the only difference is the instruction formats where we have to change how we and and or for the immediate values. This also changes the opcodes, immediates used as funct3 and funct7, and the registers. It's why we have two string arrays for the registers: reg32 and reg16. The purpose of the strings is input the register number and attain its ABI name. 

The instDecExec functions start out with a series of shifting and anding to attain the fields for the opcodes, registers, and immediates. To decode the immediates for the different instruction words we used the shifting operator as well as some masking to get the correct number of bits of immediates together and get the correct number. Immediates can be split into up to four positions in the word. Take for example the S-type immediate where Bits [4:0] are located at bit [11:7] of the instruction word and bits [11:5] are located at bit [31:25] of the instruction word. Therfore, we have to shift bits [31:25] 25 bits to the right and mask these 7 bits with 0x3F and then shift to the left 5 bits. Afterward, we concatunate with the instruction word shifted right 7 bits and masked with 0x1F.

For the immediates of addresses related to the branch and jump, we have to add to it the program counter to attain the right address. The rest is a bunch of if statements to differentiate between the opcodes and switch statements to differentiate between the funct3 and funct7 values. These three and sometimes one fields determine the specific instruction.

We have six overloaded print instructions to print the memory, opcode, and instruction code for each instruction. For each case in the switch statement, we called the print function and passed the applicable parameters. The functions are similar to the print function in the skeleton; we just added a boolean to check if it's a signed or unsigned instruction and a parameter for the immediate. 
# Limitations:
We weren't able to implement the label function due to time constraints. The same goes for the uncompressed load word using the stack.
# Known Issues:
There aren't any issues that we have encountered when trying out all the sample cases uploaded on blackboard or the test cases that we have created. The code runs smoothly as far as we are concerned.
# Contributions of each team member:
Mohamed Elkholy: code for all compressed instructions + combining the code

Salma Aly: code and test cases for uncompressed B, U, R instructions + readme file

Allaa El Khouly: code and test cases for uncompressed I, S, J instructions + test cases for compressed instructions

We all worked together to debug the errors that came up when trying the test cases
