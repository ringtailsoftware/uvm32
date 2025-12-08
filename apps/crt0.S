#include "uvm32_sys.h"
.section .initial_jump , "ax", %progbits
.global _start
.align 4
_start:
# sp is already setup by vm
sw	ra,12(sp)
jal	ra, main
#if 1
#include "non-rust-crt0-hack.S"
#else
li a7, {IOREQ_HALT}
#endif
ecall
.section .data

