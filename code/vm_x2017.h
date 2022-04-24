#ifndef VM_X2017_H
#define VM_X2017_H

#include "objdump_x2017.h"

#define MAX_RAM 256
#define Register BYTE
#define STACK_POINTER registers[6]
#define PROGRAM_COUNTER registers[7]

typedef struct stack {
    BYTE *stk_symbols;
    // The size of `stk_symbols` array
    BYTE size;
    // the address of the stack that creates the current stack (using CAL)
    BYTE return_addr;
} Stack;


typedef struct stacks {
    // The maximum number of stacks will not exceed 8,
    // so 1 byte is enough to represent all stack address. 
    BYTE size;
    // initially with size 0, resize when a new stack is added
    Stack *stack_arr;
} Stacks;


// Calculate the current RAM size 
int calculate_RAM_size(Stacks *stacks) {
    // stacks->size takes 1 byte
    int result = 1;
    for (int i = 0; i < stacks->size; i++) {
        // For each stack, stack.size and stack.return_addr takes 1 byte each.
        result += 2;
        // There are `size` stack symbols, each stack symbol taks 1 byte. 
        result = result + stacks->stack_arr[i].size;
    }
    return result;
}


int calculate_stack_size(Stack *stack) {
    return 2 + stack->size * 1;
}


// Return 0 if successfully added
// Otherwise, return -1
int add_stack(Stacks *stacks, Stack *new_stack){
    int stacks_new_size = calculate_RAM_size(stacks) + 
                        calculate_stack_size(new_stack);

    if (stacks_new_size > 256) {
        fprintf(stderr, "Run out of RAM\n");
        return -1;
    }

    stacks->stack_arr = realloc(stacks->stack_arr, (stacks->size + 1) * sizeof(Stack));
    if (stacks->stack_arr != NULL) {
        stacks->size++;
        stacks->stack_arr[stacks->size - 1] = *new_stack;
        return 0;
    } else {
        fprintf(stderr, "Cannot reallocate memory for Stacks\n");
        return -1;
    }
}

// Last in, first out
// Return the return address of the stack being removed
// Return 255 if an error has occured
BYTE rm_stack(Stacks *stacks) {
    if (stacks->size == 0) {
        fprintf(stderr, "The stack array are empty\n");
        return 255;
    }

    // The last stack must be created by the function with label 0,
    // the return address will no longer be useful. 
    if (stacks->size == 1) {
        stacks->size = 0;
        return 0;
    }

    BYTE return_addr = stacks->stack_arr[stacks->size - 1].return_addr;

    free(stacks->stack_arr[stacks->size - 1].stk_symbols);
    stacks->stack_arr = realloc(stacks->stack_arr, (stacks->size - 1) * sizeof(Stack));
    if (stacks->stack_arr != NULL){
        stacks->size--;
        return return_addr;
    } else {
        fprintf(stderr, "Cannot reallocate memory for Stacks\n");
        return 255;
    }
}

// Get the first three bits of a 8-bit pointer
// The first three bits stores the stack address
BYTE get_first_3_bits(BYTE ptr) {
    return (ptr & 0b11100000) >> 5;
}

// Get the last five bits of a 8-bit pointer
// The last five bits stores the address of a stack symbol 
BYTE get_last_5_bits(BYTE ptr) {
    return (ptr & 0b00011111);
}

BYTE set_first_3_bits(BYTE *ptr, BYTE new_3_bits) {
    *ptr = get_last_5_bits(*ptr) + (new_3_bits << 5);
    return *ptr;
}

BYTE set_last_5_bits(BYTE *ptr, BYTE new_5_bits) {
    *ptr = (*ptr & 0b11100000) + new_5_bits;
    return *ptr;
}


// **************** Functions below are copied from objdump_x2017.c ****************

// By given a bit_idx, we are able to find which bit of which byte it is. 
BYTE get_bit(BYTE* byte_array, int bit_idx){
    // reminder is the number of first few bits in the current byte
    int reminder = bit_idx % 8;
    // 7 - reminder is to skip the bits that have been accessed
    int shift = 7 - reminder;
    int byte_idx = (bit_idx - reminder) / 8;
    return (byte_array[byte_idx] & (1 << shift)) != 0;
}

// Get n bits from the end of the byte (both inclusive)
BYTE get_bits(BYTE* byte_array, int n, int bit_idx){

    BYTE result = 0;
    for (int i = 0; i < n; i++){
        result += get_bit(byte_array, bit_idx--) << i;
    }
    return result;
}

// Return the number of bits of the given type `b`
int get_type_bits(BYTE type) {
    int type_bits = 0;
    switch (type)
    {
    case VALUE:
        type_bits = 8;
        break;
    case REGISTER:
        type_bits = 3;
        break;
    case STACK:
        type_bits = 5;
        break;
    case POINTER:
        type_bits = 5;
        break;
    default: // If `type` matches none of above, then it is invalid.
        type_bits = -1;
        break;
    }
    return type_bits;
}


Function* parse(const char* filename){
    FILE *file = fopen(filename, "rb"); 
    if (file == NULL) {
        perror("unable to open file");
        return NULL; 
    }

    fseek(file, 0L, SEEK_END);
    int length = ftell(file);
    // Move the pointer back to the head of the file
    fseek(file, 0, SEEK_SET);
    // byte_array stores all the bytes in the file
    BYTE *byte_array = (BYTE*) malloc(length * sizeof(BYTE));

    BYTE byte;
    int idx = 0;
    while (fread(&byte, sizeof(BYTE), 1, file) != 0){
        byte_array[idx] = byte;
        idx++;
    }

    fclose(file);


    // `func_list` is the head of the linked list 
    // storing all the functions in the file
    Function *func_list = NULL;   

    // the current bit we are to read
    int bit_idx = 8 * length - 1;

    // Parse the functions in the file.
    // Each function at least has a "label" (3 bits), a "RET" (3 bits),
    // and a "Number of instructions" (5 bits), which is 11 bits in total.
    // Hence, the minimum size of a function is 11 bits.
    // If bit_idx less than 11, it means no more functions in the file.
    while (bit_idx >= 11) {

        // Every iteration produces a new function

        // the total number of instructions in a function
        int total_ins = 0;

        // Parse the number of instructions in the function (5 bits)
        total_ins = get_bits(byte_array, 5, bit_idx);
        bit_idx -= 5;

        // the next new function to be added into func_list
        Function *new_func = (Function*)malloc(sizeof(Function));
        // stores all the instructions in the function
        new_func->ins_arr = (Instruction*) malloc(total_ins * sizeof(Instruction));
        new_func->total_ins = total_ins;
        new_func->symbol_cnt = 0;
        
        // Parse the operations in the function
        for (int ins_cnt = total_ins - 1; ins_cnt >= 0; ins_cnt--){

            // Every iteration produces a new instruction

            // Parse the opcode
            BYTE opcode = get_bits(byte_array, 3, bit_idx);
            Instruction new_ins = {.opcode = opcode};
            bit_idx -= 3;

            // RET has no arguments
            // the other operations have at least 1 argument
            if (opcode != RET) { 
                // parse the type
                new_ins.type1 = get_bits(byte_array, 2, bit_idx);
                bit_idx -= 2;

                int type_bits = get_type_bits(new_ins.type1);
                if (type_bits == -1) {
                    printf("invalid type");
                    return NULL;
                } 

                // parse the value
                new_ins.value1 = get_bits(byte_array, type_bits, bit_idx);
                bit_idx -= type_bits;

                // these operations have the second argument
                if (opcode == MOV || opcode == REF || opcode == ADD) {
                    // parse the type
                    new_ins.type2 = get_bits(byte_array, 2, bit_idx);
                    bit_idx -= 2;

                    type_bits = get_type_bits(new_ins.type2);
                    if (type_bits == -1) {
                        printf("invalid type");
                        return NULL;
                    } 

                    // parse the value
                    new_ins.value2 = get_bits(byte_array, type_bits, bit_idx);
                    bit_idx -= type_bits;
                }
            }
            new_func->ins_arr[ins_cnt] = new_ins;
        }

        // parse function label
        new_func->label = get_bits(byte_array, 3, bit_idx);
        bit_idx -= 3;
        
        // add the new_func to the head of func_list
        new_func->next_func = func_list;
        func_list = new_func;
    }

    free(byte_array);
    return func_list;
}


void free_all_func(Function *func_list) {
    while (func_list != NULL) {
        free(func_list->ins_arr);
        Function *temp = func_list->next_func;
        free(func_list);
        func_list = temp;
    }
}


#endif