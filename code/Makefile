CC=gcc
CFLAGS=-fsanitize=address -Wvla -Wall -Werror -g -std=gnu11 -lasan

# fill in all your make rules

vm_x2017: vm_x2017.c
	$(CC) $(CFLAGS) $^ -o vm_x2017
	

objdump_x2017: objdump_x2017.c 
	$(CC) $(CFLAGS) $^ -o objdump_x2017


run_tests: vm_x2017
	bash test.sh

clean:
	rm vm_x2017

