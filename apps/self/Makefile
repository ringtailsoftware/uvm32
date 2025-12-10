TOPDIR=../../
PROJECT:=$(shell basename ${PWD})
SRCS=${PROJECT}.c ${TOPDIR}/apps/crt0.S ${TOPDIR}/uvm32/uvm32.c
CFLAGS=-DUVM32_MEMORY_SIZE=8192 -DCUSTOM_STDLIB_H=\"uvm32_target.h\" -I../../uvm32
all: all_common
test: test_common
clean: clean_common
include ${TOPDIR}/apps/makefile.common
