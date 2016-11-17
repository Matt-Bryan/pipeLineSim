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
#define SW 5

#define HALT -1
#define CLEAR -2

#define EMPTY 100
#define BUSY 101

typedef unsigned int MIPS, *MIPS_PTR;

MB_HDR mb_hdr;		/* Header area */
static MIPS mem[1024];		/* Room for 4K bytes */
static unsigned int PC;
static int reg[32], numInstr;
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
         res = reg[rs] + immVal;
         break;
      case 0x09: //add immediate unsigned
         res = (unsigned int)reg[rs] + (unsigned int)immVal;
         break;
      case 0x0C: //and immediate
         res = reg[rs] & immVal;
         break;
      case 0x0D: //or immediate
         res = reg[rs] | immVal;
         break;
      case 0x0E: //xor immediate
         res = reg[rs] ^ immVal;
         break;
      case 0x0A: //set less than signed
         rs = reg[rs] < immVal;
         break;
      case 0x0B: //set less than unsigned
         res = (unsigned int)reg[rs] < (unsigned int)immVal;
         break;
      case 0x2B: //store word
         res = reg[rs] + immVal;
         break;
      case 0x0F: //lui
         res = immVal << 16;
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
         res = reg[rt] << shamt;
         break;
      case 0x02 : 	// srl
      	 unsignedVal1 = (unsigned int) reg[rt];
         res = unsignedVal1 >> shamt;
         break;
      case 0x03 : 	// sra
         signedVal1 = (int) reg[rt];
         res = signedVal1 >> shamt;
         break;
      case 0x04 : 	// sllv
         res = reg[rt] << reg[rs];
         break;
      case 0x06 : 	// srlv
         res = reg[rt] >> reg[rs];
         break;
      case 0x07 : 	// srav
         signedVal1 = (int) reg[rt];
         res = signedVal1 >> reg[rs];
         break;
      case 0x20 : 	// add
         signedVal1 = reg[rs];
         signedVal2 = reg[rt];
         res = signedVal1 + signedVal2;
         break;
      case 0x21 : 	// addu
         res = (unsigned int)reg[rs] + (unsigned int)reg[rt];
         break;
      case 0x22 : 	// sub
         signedVal1 = reg[rs];
         signedVal2 = reg[rt];
         res = signedVal1 - signedVal2;
         break;
      case 0x23 : 	// subu
         res = (unsigned int)reg[rs] - (unsigned int)reg[rt];
         break;
      case 0x24 : 	// and
         res = reg[rs] & reg[rt];
         break;
      case 0x25 : 	// or
         res = reg[rs] | reg[rt];
         break;
      case 0x26 : 	// Xor
         res = reg[rs] ^ reg[rt];
         break;
      case 0x27 : 	// Nor
         res = ~(reg[rs] | reg[rt]);
         break;
      case 0x2A : 	// slt
         signedVal1 = (int) reg[rs];
         signedVal2 = (int) reg[rt];
         res = (signedVal1 < signedVal2);
         break;
      case 0x2B : 	// sltu
         res = (unsigned int)rs < (unsigned int)reg[rt];
         break;
      case 0x08 : 	// jr
         PC = reg[rs];
         return CLEAR;
      case 0x09 : 	// jalr
         reg[31] = PC;
         PC = reg[rs];
         return CLEAR;
   }
   
   return res;
}

int exBr(int op, int rs, int rt, int brAdd) {
   switch (op) {
      case 0x04: //branch equal
         if (reg[rs] == reg[rt])
         {
            PC = brAdd;
            return CLEAR;
         }
         break;
      case 0x05: //branch not equal
         if (reg[rs] != reg[rt])
         {
            PC = brAdd;
            return CLEAR;
         }
         break;
   }
   return 0;
}

int exLSW(int rs, int immVal) {
   int res;
   res = reg[rs] + immVal;
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
      
      reg[31] = PC - 4;
      PC = (PC & 0xF0000000) | jmpAdd;
   }
}

void writeBack(int *memOut) {
   reg[memOut[0]] = memOut[1];
   numInstr++;
}

void memRef(int *exOut, int *memOut, int type) 
{
   /* memOut format: { rd, MDR, flagBit } */
   if (type == LW)
   {
       memOut[0] = exOut[0];
       memOut[1] = mem[(exOut[1]) / 4];
       memOut[2] = 1;
   }
   else if (type == SW)
   {
      memOut[0] = 0;
      memOut[1] = 0;
      mem[(exOut[1]) / 4] = reg[(exOut[0])];
      memOut[2] = 0;
      numInstr++;
   }
   else
   {
      memOut[0] = 0;
      memOut[1] = 0;
      reg[exOut[0]] = exOut[1];
      memOut[2] = 0;
      numInstr++;
   }
}

void execute(int *idOut, int *exOut, int type, int *brAddress)
{
   /* exOut format: { rd, ALUresult, flagBit } */
   int result, temp;

   /* FORMAT: {op(0), rs(1), rt(2), rd(3), imm(4), shamt(5), jmpIdx(6), func(7)} */
   switch(type)
   {
      case HALT:
         result = HALT;
         exOut[0] = HALT;
         break;
      case REG:
         temp = exReg(idOut[1], idOut[2], idOut[5], idOut[7]);
         if (temp == CLEAR) {
            exOut[0] = 0;
            result = 0;
            exOut[2] = CLEAR;
         }
         else {
            result = temp;
            exOut[0] = idOut[3];
            exOut[2] = 1;
         }
         break;
      case IMM:
         result = exImm(idOut[1], idOut[0], idOut[4]);
         exOut[0] = idOut[2];
         exOut[2] = 1;
         break;
      case JMP:
         result = 0;
         exOut[2] = CLEAR;
         exJmp(idOut[0], idOut[6]);
         numInstr++;
         break;
      case BR:
         if (exBr(idOut[0], idOut[1], idOut[2], *brAddress) == CLEAR) {
            exOut[0] = 0;
            result = 0;
            exOut[2] = CLEAR;
         }
         numInstr++;
         break;
      case LW:
         result = exLSW(idOut[1], idOut[4]);
         exOut[0] = idOut[2];
         exOut[2] = 1;
         break;
      case SW:
         result = exLSW(idOut[1], idOut[4]);
         exOut[0] = idOut[2];
         exOut[2] = 1;
         break;
   }

   /* NEED TO DO: READY ARRAY FOR OUTPUT */
   exOut[1] = result;
}

void decode(int *ifOut, int *idOut, int *type, int *brAddress) {
   int op = (ifOut[0] & OPMASK) >> 26;
   int imm = (ifOut[0] & IMMASK);
   int shamt = (ifOut[0] & SHMASK) >> 6;
   int func = (ifOut[0] & FNMASK);
   int jmpIdx = (ifOut[0] & JIMASK) << 2;

   int rs = (ifOut[0] & RSMASK) >> 21;
   int rt = (ifOut[0] & RTMASK) >> 16;
   int rd = (ifOut[0] & RDMASK) >> 11;



   if ((ifOut[0] & 0x00008000)) /* check the sign bit */
   {
      imm |= 0xFFFF0000;
   }

   /* Determine the instruction type */
   switch (op)
   {
      case 0x0:
         if (func == 0xC) {
            *type = HALT;
            ifOut[1] = HALT;
         }
         else
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
      case 0x2B:
         *type = SW;
         break;
      default:
         *type = IMM;
         break;
   }

   /* ALUOut gets branch eff address */
   *brAddress = PC + (imm << 2);

   /* set idOut to the output array; format: { op, rs, rt, rd, imm, shamt, jmpIdx, func, flagBit} */
   idOut[0] = op;
   idOut[1] = rs;
   idOut[2] = rt;
   idOut[3] = rd;
   idOut[4] = imm;
   idOut[5] = shamt;
   idOut[6] = jmpIdx;
   idOut[7] = func;
   
}

void fetch(int *ifOut)
{
   ifOut[0] = mem[PC/4];
   PC = PC + 4;
}

void displayResult() {
   int count = 0;
   
   for (count; count < 32; count++)
   {
      printf("R%d = %08X\n", count, reg[count]);
   }
   printf("PC: %08X\n", PC);
   printf("Total Clock Cycles: %0.2f\n", clockCount);
   printf("Number of Instructions completed: %d\n", numInstr);
   printf("CPI: %0.2f\n", clockCount / numInstr);
   
}

int main(int argc, char **argv) {
   int memp, input = 0, numInstr = 0, exitFlag = 0;
   int ifOut[] = {0, 0}; /* format: {32 bit instruction, flagBit}*/		/* flagBit format: 0 = DIDNT RUN, 1 = RAN */
   int idOut[] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; /* format: { op, rs, rt, rd, imm, shamt, jmpIdx, func, flagBit } */
   int exOut[] = {0, 0, 0}; /* format: { rd, ALUResult, flagBit } */
   int memOut[] = {0, 0, 0}; /* format: {rd, MDR, flagBit} */
   int type, brAddress;

	PC = 0;
	clockCount = 0.0;

	memp = loadMemory(argv[1]);

    printf("Enter 0 for Run or 1 for Single-step: ");
    scanf("%02d", &input);

	do {
		if (memOut[2] == 1) {
			writeBack(memOut);
		}
 		if (exOut[2] == 1) {
 			memRef(exOut, memOut, type);
 		}
        else {
            memOut[2] = 0;
        }
		if (idOut[8] == 1) {
			execute(idOut, exOut, type, &brAddress);
		}
        else {
            exOut[2] = 0;
        }
        if (exOut[2] == CLEAR)
        {
            /* Clearing involves flushing in/outboxes and setting idFlag to busy */
            ifOut[0] = 0;
            ifOut[1] = 0;
        }
        if (ifOut[1] == 1) {
        	decode(ifOut, idOut, &type, &brAddress);
        	idOut[8] = 1;
        }
        else {
            idOut[8] = 0;
        }
        if (ifOut[1] != HALT) {
        	fetch(ifOut);
            ifOut[1] = 1;
                // if (ifOut[0] == 0)
                // {
                //    ifOut[1] = 0;
                //    printf("FUUUUUUUCK\n");
                // }
                // else 
                // {
        	       // ifOut[1] = 1;
                // }
        }
        else {
            exitFlag++;
        }
        if (input == 1 && exitFlag < 4) {
            displayResult();
            printf("Enter 0 for Run or 1 for Single-step: ");
            scanf("%02d", &input);
        }
        clockCount++;
	} while (exitFlag < 4);

	// for (i = 0; i < memp; i += 4) {
 //       printf("Instruction@%08X : %08X\n", i, mem[i/4]);
 //    }
    displayResult();
	return 0;
}
