TOPDIR=../../../
PROJECT:=$(shell basename ${PWD})
SRCS=${PROJECT}.c ${TOPDIR}/apps/crt0.S
all: all_common
	@# Convert ROM to C file and header
	@xxd -i ${PROJECT}.bin > ${PROJECT}-header.c
	@echo "extern unsigned char ${PROJECT}_bin[]; extern int ${PROJECT}_bin_len;" > ${PROJECT}-header.h

test: test_common
clean: clean_common
	rm -f ${PROJECT}-header.h ${PROJECT}-header.c

include ${TOPDIR}/apps/makefile.common
