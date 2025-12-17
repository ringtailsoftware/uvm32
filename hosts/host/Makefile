TOPDIR=../..

all:
	gcc -g -Wall -Werror -Wno-gnu-binary-literal -O0 -DUVM32_ERROR_STRINGS -DUVM32_MEMORY_SIZE=65536 -I${TOPDIR}/uvm32 -I${TOPDIR}/common -o host ${TOPDIR}/uvm32/uvm32.c host.c

clean:
	rm -f host
