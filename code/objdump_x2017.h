#ifndef OBJDUMP_X2017_H
#define OBJDUMP_X2017_H

#include <stdio.h>
#include <stdlib.h>

#define BYTE unsigned char

#define STACK_LENGTH 32
#define MAX_8_BITS_NUM 255

#define MOV 0b000
#define CAL 0b001
#define RET 0b010
#define REF 0b011
#define ADD 0b100
#define PRINT 0b101
#define NOT 0b110
#define EQU 0b111

#define VALUE 0b00
#define REGISTER 0b01
#define STACK 0b10
#define POINTER 0b11


typedef struct instruction{
    unsigned opcode: 3;
    unsigned type1: 2;
    unsigned type2: 2;
    unsigned value1: 8;
    unsigned value2: 8;
} Instruction;

typedef struct function {
    int label;
    int total_ins; // stores the number of instructions in the funciton
    BYTE *stk_symbol;
    int symbol_cnt;
    Instruction *ins_arr;
    struct function *next_func;
} Function;

#endif
