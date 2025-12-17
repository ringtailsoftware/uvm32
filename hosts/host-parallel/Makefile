TOPDIR = ../..
all:
	@echo "const unsigned char fib[] = {" > fib.h
	@cat ${TOPDIR}/precompiled/fib.bin | xxd -i >> fib.h
	@echo "};\nunsigned int fib_len = " >> fib.h
	@wc -c ${TOPDIR}/precompiled/fib.bin | awk '{print $$1}' >> fib.h
	@echo ";" >> fib.h
	gcc -Wall -DUVM32_ERROR_STRINGS -DUVM32_MEMORY_SIZE=16386 -Wno-gnu-binary-literal -I${TOPDIR}/uvm32 -I${TOPDIR}/common -o host-parallel ${TOPDIR}/uvm32/uvm32.c host-parallel.c

clean:
	rm -f host-parallel
