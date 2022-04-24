#include <stdio.h>
#include <stdlib.h>
#include "objdump_x2017.h"

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

// Map a stack symbol to a character(i.e. A~Z, a~e)
char byte_to_char(BYTE byte) {
    int increment;

    if (byte <= 25) {
        // 'A' has ASCII code 65
        increment = 65;
    } else { // byte > 25, use a ~ e
        increment = 97;
    }

    return (byte + increment);
}


// Map `BYTE type` to `char* type`
char* get_type(BYTE type) {
    char *result;
    switch (type)
    {
    case VALUE:
        result = "VAL";
        break;
    case REGISTER:
        result = "REG";
        break;
    case STACK:
        result = "STK";
        break;
    case POINTER: 
        result = "PTR";
        break;
    default:
        result = NULL;
        break;
    }
    return result;
}

// Map the opcodes to operation strings
char* get_opt(BYTE opcode) {
    char *result;
    switch (opcode)
    {
    case MOV:
        result = "MOV";
        break;
    case CAL:
        result = "CAL";
        break;
    case RET:
        result = "RET";
        break;
    case REF:
        result = "REF";
        break;
    case ADD:
        result = "ADD";
        break;
    case PRINT:
        result = "PRINT";
        break;
    case NOT:
        result = "NOT";
        break;
    case EQU:
        result = "EQU";
        break;
    default:
        result = NULL;
        break;
    }
    return result;
}  


// Check if the given opcode is valid.
// Return 1 if it is valid,
// otherwise return -1.
int is_valid_opcode(BYTE opcode) {
    if (opcode != ADD && opcode != MOV && opcode != CAL &&
        opcode != RET && opcode != REF && opcode != PRINT && 
        opcode != NOT && opcode != EQU) {
            return -1;
    }
    return 1;
}


int is_valid_type(BYTE type) {
    if (type != VALUE && type != REGISTER && type != POINTER && type != STACK) {
        return -1;
    }
    return 1;
}


// If an error occurs, return NULL
Function* parse(const char* filename){

    FILE *file = fopen(filename, "rb"); 

    if (file == NULL) {
        perror("unable to open file");
        return NULL; 
    }

    // Obtain the length of the file 
    // Move the pointer to the end of the file
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

    // the current bit to read
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
        new_func->next_func = NULL;

        // Parse the operations in the function
        for (int ins_cnt = total_ins - 1; ins_cnt >= 0; ins_cnt--){

            // Every iteration produces a new instruction

            // Parse the opcode
            BYTE opcode = get_bits(byte_array, 3, bit_idx);
            Instruction new_ins = {.opcode = opcode};
            bit_idx -= 3;

            if (is_valid_opcode(opcode) == -1) {
                fprintf(stderr, "Invalid operation code\n");
                return NULL;
            }

            // RET has no arguments
            // the other operations have at least 1 argument
            if (opcode != RET) { 
                // parse the type
                new_ins.type1 = get_bits(byte_array, 2, bit_idx);
                bit_idx -= 2;

                if (is_valid_type(new_ins.type1) == -1) {
                    fprintf(stderr, "The first argument has invalid type\n");
                    return NULL;
                }

                int type_bits = get_type_bits(new_ins.type1);
                if (type_bits == -1) {
                    printf("invalid type\n");
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

                    if (is_valid_type(new_ins.type2) == -1) {
                        fprintf(stderr, "The second argument has invalid type\n");
                        return NULL;
                    }

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


void print_functions(Function *pf) {
    while (pf != NULL) {
        // The indices represents stack symbols and the value represents
        // the corrosponding capital letter
        // Initialise all the elements to MAX_8_BITS_NUM
        pf->stk_symbol = (BYTE*)malloc(STACK_LENGTH * sizeof(char));
        for (int i = 0; i < STACK_LENGTH; i++) {
            pf->stk_symbol[i] = MAX_8_BITS_NUM;
        }

        printf("FUNC LABEL %d\n", pf->label);

        Instruction *pi = pf->ins_arr;
        for (int i = 0; i < pf->total_ins; i++) {
            printf("    ");
            printf("%s", get_opt(pi[i].opcode));
            if (pi[i].opcode != RET) {

                printf(" %s ", get_type(pi[i].type1));
                if (pi[i].type1 == STACK || pi[i].type1 == POINTER) {
                    if (pf->stk_symbol[pi[i].value1] == MAX_8_BITS_NUM) {
                        pf->stk_symbol[pi[i].value1] = byte_to_char(pf->symbol_cnt);
                        pf->symbol_cnt++;
                    }

                    printf("%c", pf->stk_symbol[pi[i].value1]);
                } else {
                    printf("%d", pi[i].value1);
                }
                // these operations have the second argument
                if (pi[i].opcode == MOV || pi[i].opcode == ADD 
                    || pi[i].opcode == REF) {
                    printf(" %s ", get_type(pi[i].type2));

                    if (pi[i].type2 == STACK || pi[i].type2 == POINTER) {
                        if (pf->stk_symbol[pi[i].value2] == MAX_8_BITS_NUM) {
                            pf->stk_symbol[pi[i].value2] = byte_to_char(pf->symbol_cnt);
                            pf->symbol_cnt++;
                        }

                        printf("%c", pf->stk_symbol[pi[i].value2]);
                    } else {
                        printf("%d", pi[i].value2);
                    }
                }
            }
            printf("\n");
        }
        
        pf = pf->next_func;
    }
}


void free_all_func(Function *func_list) {
    while (func_list != NULL) {
        free(func_list->stk_symbol);
        free(func_list->ins_arr);
        Function *temp = func_list->next_func;
        free(func_list);
        func_list = temp;
    }
}


int main(int argc, char const *argv[])
{   
    Function *func_list = parse(argv[1]);
    print_functions(func_list);
    free_all_func(func_list);
    
    return 0;
}