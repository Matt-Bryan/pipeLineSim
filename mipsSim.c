/*----------------------------------------------------------------------*
 *	Example mips_asm program loader. This loads the mips_asm binary	*
 *	named "testcase1.mb" into an array in memory. It reads		*
 *	the 64-byte header, then loads the code into the mem array.	*
 *									*
 *	DLR 4/18/16							*
 *
 *
 *  This file will be modified to act as a fully-fledged MIPS simulator.
 *----------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mips_asm_header.h"

#define OPMASK 0xFC000000	//Right shift 26
#define RSMASK 0x03E00000   //21
#define RTMASK 0x001F0000   //16
#define RDMASK 0x0000F800   //11
#define SHMASK 0x000007C0   //6
#define FNMASK 0x0000003F
#define IMMASK 0x0000FFFF
#define JIMASK 0x03FFFFFF
#define SYMASK 0xFFFFFFF0

#define BYTEMASK 0x000000FF //for load and store bytes
#define SBYTEMASK 0x0000FF00 
#define TBYTEMASK 0x00FF0000
#define FRBYTEMASK 0xFF000000
#define HALFMASK 0x0000FFFF // for half words
#define SHALFMASK 0xFFFF0000

#define REG 0
#define IMM 1
#define JMP 2
#define BR 3
#define LW 4

typedef unsigned int MIPS, *MIPS_PTR;

MB_HDR mb_hdr;		/* Header area */
static MIPS mem[1024];		/* Room for 4K bytes */
static unsigned int PC, *nextInstruction;
static int reg[32];
static float clockCount;


int loadMemory(char *filename)	//This function acts as step 1 of lab 5, and loads the MIPS binary file into an array
  {
  FILE *fd;
  int n;
  int memp;
/* format the MIPS Binary header */

  fd = fopen(filename, "rb");
  if (fd == NULL) { printf("\nCouldn't load test case - quitting.\n"); exit(99); }

  memp = 0;		/* This is the memory pointer, a byte offset */

/* read the header and verify it. */
  fread((void *) &mb_hdr, sizeof(mb_hdr), 1, fd);
  if (! strcmp(mb_hdr.signature, "~MB")==0)
    { printf("\nThis isn't really a mips_asm binary file - quitting.\n"); exit(98); }
  
  printf("\n%s Loaded ok, program size=%d bytes.\n\n", filename, mb_hdr.size);

/* read the binary code a word at a time */
  
  do {
    n = fread((void *) &mem[memp/4], 4, 1, fd); /* note div/4 to make word index */
    if (n) 
      memp += 4;	/* Increment byte pointer by size of instr */
    else
      break;       
    } while (memp < sizeof(mem)) ;

  fclose(fd);

  return memp;
  }


/*Function that handles I type instructions */
int exImm(int rs, int op, int immVal) {
   int res;
   
   switch (op) { //switching on the op code
      case 0x08: //add immediate
         res = rs + immVal;
         break;
      case 0x09: //add immediate unsigned
         res = (unsigned int)rs + (unsigned int)immVal;
         break;
      case 0x0C: //and immediate
         res = rs & immVal;
         break;
      case 0x0D: //or immediate
         res = rs | immVal;
         break;
      case 0x0E: //xor immediate
         res = rs ^ immVal;
         break;
      case 0x0A: //set less than signed
         rs = rs < immVal;
         break;
      case 0x0B: //set less than unsigned
         res = (unsigned int)rs < (unsigned int)immVal;
         break;
      case 0x2B: //store word
         res = rs + immVal;
         break;
    }
   
   return res;
}

/* Sub routine of execute to handle register-type instructions */
int exReg(int rs, int rt, int shamt, int func)
{
   int signedVal1 = 0;
   int signedVal2 = 0;
   unsigned int unsignedVal1 = 0;
   unsigned int unsignedVal2 = 0;
   int res;

   switch(func) 
   {
      case 0x00 : 	// sll
         res = rt << shamt;
         break;
      case 0x02 : 	// srl
      	 unsignedVal1 = (unsigned int) rt;
         res = unsignedVal1 >> shamt;
         break;
      case 0x03 : 	// sra
         signedVal1 = (int) rt;
         res = signedVal1 >> shamt;
         break;
      case 0x04 : 	// sllv
         res = rt << rs;
         break;
      case 0x06 : 	// srlv
         res = rt >> rs;
         break;
      case 0x07 : 	// srav
         signedVal1 = (int) rt;
         res = signedVal1 >> rs;
         break;
      case 0x20 : 	// add
         signedVal1 = rs;
         signedVal2 = rt;
         res = signedVal1 + signedVal2;
         break;
      case 0x21 : 	// addu
         res = (unsigned int)rs + (unsigned int)rt;
         break;
      case 0x22 : 	// sub
         signedVal1 = rs;
         signedVal2 = rt;
         res = signedVal1 - signedVal2;
         break;
      case 0x23 : 	// subu
         res = (unsigned int)rs - (unsigned int)rt;
         break;
      case 0x24 : 	// and
         res = rs & rt;
         break;
      case 0x25 : 	// or
         res = rs | rt;
         break;
      case 0x26 : 	// Xor
         res = rs ^ rt;
         break;
      case 0x27 : 	// Nor
         res = ~(rs | rt);
         break;
      case 0x2A : 	// slt
         signedVal1 = (int) rs;
         signedVal2 = (int) rt;
         res = (signedVal1 < signedVal2);
         break;
      case 0x2B : 	// sltu
         res = (unsigned int)rs < (unsigned int)rt;
         break;
      case 0x08 : 	// jr
         PC = rs;
         break;
      case 0x09 : 	// jalr
         reg[31] = PC + 4;
         PC = rs;
         break;
   }
   
   return res;
}

void exBr(int op, int rs, int rt, int brAdd) {
   switch (op) {
      case 0x04: //branch equal
         if (rs == rt)
            PC = brAdd;
         break;
      case 0x05: //branch not equal
         if (rs != rt)
            PC = brAdd;
         break;
   }
}

int exLw(int rs, int immVal) {
   int res;
   
   res = rs + immVal;
   return res;
}

/* Takes the location in memory of the next instruction as an argument.
 *
 *  Modifies the nextInstruction buffer with details about the next Instruction
 *
 *  If it detects a syscall halt, it only puts -1 in first spot of buffer  -MB*/

void exJmp(int op, int jmpAdd) {
   if (op == 0x02) {
      //J
      
      PC = (PC & 0xF0000000) | jmpAdd;
   }
   else {
      //Jal
      
      reg[31] = PC;
      PC = (PC & 0xF0000000) | jmpAdd;
   }
}

void writeBack() {

}

void memRef() {

}

void execute(int *idOut, int *exOut, int type, int *brAddress)
{
   int output[] = {0, 0}; /* format: {rd, ALUresult} */
   int result;

   /* FORMAT: {op(0), rs(1), rt(2), rd(3), imm(4), shamt(5), jmpIdx(6), func(7)} */
   switch(type)
   {
      case REG:
         result = exReg(idOut[1], idOut[2], idOut[5], idOut[7]);
         output[0] = idOut[3];
         break;
      case IMM:
         result = exImm(idOut[1], idOut[0], idOut[4]);
         output[0] = idOut[2];
         break;
      case JMP:
         result = exJmp(idOut[0], idOut[6]);
         break;
      case BR:
         result = exBr(idOut[0], idOut[1], idOut[2], *brAddress);
         output[0] = idOut[2];
         break;
      case LSW:
         result = exLSW(idOut[1], idOut[4]);
         output[0] = idOut[2];
         break;
   }

   /* NEED TO DO: READY ARRAY FOR OUTPUT */
   output[1] = result;

   exOut = output;
}

void decode(int *ifOut, int *idOut, int *type, int *brAddress) {
   int output[] = {0, 0, 0, 0, 0, 0, 0, 0}; /* format: {op, rs, rt, rd, imm, shamt, jmpIdx, func}
   int op = (*ifOut & OPMASK) >> 26;
   int imm = (*ifOut & IMMASK);
   int shamt = (*ifOut & SHMASK) >> 6;
   int func = (*ifOut & FNMASK);
   int jmpIdx = (*ifOut & JIMASK) << 2;

   *rs = (*ifOut & RSMASK) >> 21;
   *rt = (*ifOut & RTMASK) >> 16;
   *rd = (*ifOut & RDMASK) >> 11;
 
   if ((*ifOut & 0x00008000)) /* check the sign bit */
   {
      imm |= 0xFFFF0000;
   }

   /* Determine the instruction type */
   switch (op)
   {
      case 0x0:
         *type = REG;
         break;
      case 0x2:
         *type = JMP;
         break;
      case 0x3:
         *type = JMP;
         break;
      case 0x4:
         *type = BR;
         break;
      case 0x5:
         *type = BR;
         break;
      case 0x23:
         *type = LW;
         break;
      default:
         *type = IMM;
         break;
   }

   /* ALUOut gets branch eff address */
   *brAddress = PC + (imm << 2);

   /* set idOut to the output array; format: [op, rs, rt, rd, imm, shamt, jmpIdx, func] */
   output[0] = op;
   output[1] = *rs;
   output[2] = *rt;
   output[3] = *rd;
   output[4] = imm;
   output[5] = shamt;
   output[6] = jmpIdx;
   output[7] = func;

   idOut = output; 
}

void fetch(int *ifOut)
{
   *ifOut = mem[PC/4];
   PC = PC + 4;
}

void displayResult() {
	
}

int main(int argc, char **argv) {
	int memp, input = 0, numInstr = 0, exitFlag = 0;
   int ifOut, ifFlag;
   int idOut, idFlag;
   int exOut, exFlag;
   int memOut, memFlag;
   int wbFlag;
   int type, brAddress;

	nextInstruction = calloc(6, sizeof(int));
	PC = 0;
	clockCount = 0.0;

	memp = loadMemory(argv[1]);

	do {
		writeBack();
		memRef();
		execute(&idOut, &exOut, type, &brAddress);
		decode(&ifOut, &idOut, &type, &brAddress);
		fetch(&ifOut);
	} while (exitFlag == 0);

	// for (i = 0; i < memp; i += 4) {
 //       printf("Instruction@%08X : %08X\n", i, mem[i/4]);
 //    }

	free(nextInstruction);
	return 0;
}
