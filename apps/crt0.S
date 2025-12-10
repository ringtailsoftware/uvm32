#include "uvm32_sys.h"

.equ uvm32_syscall_halt,          0x1000000
.equ uvm32_syscall_yield,         0x1000001
.equ uvm32_syscall_stackprotect,  0x1000002

.section .initial_jump , "ax", %progbits
.global _start
.align 4
_start:

la a0, _estack
li a7, uvm32_syscall_stackprotect
ecall

# sp is already setup by vm
sw	ra,12(sp)
jal	ra, main

li a7, uvm32_syscall_halt
ecall

.section .data

