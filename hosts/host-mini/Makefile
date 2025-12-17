TOPDIR=../..

all:
	@echo "const unsigned char mandel[] = {" > mandel.h
	@cat ${TOPDIR}/precompiled/mandel.bin | xxd -i >> mandel.h
	@echo "};\nunsigned int mandel_len = " >> mandel.h
	@wc -c ${TOPDIR}/precompiled/mandel.bin | awk '{print $$1}' >> mandel.h
	@echo ";" >> mandel.h
	gcc -Wall -DUVM32_ERROR_STRINGS -DUVM32_MEMORY_SIZE=512 -Wno-gnu-binary-literal -O2 -I${TOPDIR}/uvm32 -I${TOPDIR}/common -o host-mini ${TOPDIR}/uvm32/uvm32.c host-mini.c

clean:
	rm -f host-mini
