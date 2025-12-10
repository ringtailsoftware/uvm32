all:
	gcc -Wall -Werror -pedantic -std=c99 -O2 -DUVM32_MEMORY_SIZE=65535 -I../uvm32 -I../common -o host ../uvm32/uvm32.c host.c

clean:
	rm -f host
