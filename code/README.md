## Description
This project implements and perform operations on a simple virtual machine. The project will emulate a virtual machine to store and reference variables and stack frame contexts, and then read a set of pseudo assembly instructions that dictate the operations that should be performed on the stack. \
The arcrchitecture and the program code are defined in `assignment2.pdf`. 

## Test
A brief description for each test case is given below.

### Positive Testcase
Positive1:
    A positive test case without error produced
Positive2:
    A positive test case without error produced


### Negative Testcase 
cpy_to_val:
    Copy the value in STK A to the type VAL 

func_not_found:
    The entry function with label 0 is missing

stk_not_exist:
    Assign a value to Stack Symbol A. Treat this value as an address
    and print the value stored at that address

ref_typr_err:
    Reference the address a value type to register 0

add_type_err:
    Add two values from stack symbols

not_type_err:
    Perform NOT operation on stack symbol value

equ_type_err:
    Perform EQU on stack symbol value
