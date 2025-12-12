#include "uvm32_target.h"
#include "../shared.h"

void main(void) {
    switch(syscall(SYSCALL_PICKTEST, 0, 0)) {
        case TEST1: {
            uint32_t *p = (uint32_t *)UVM32_EXTRAM_BASE;

            // read memory and print via syscall
            printdec(*p);
            // modify memory
            *p = *p * 2;
        } break;
        case TEST2: {
            uint32_t *p = (uint32_t *)UVM32_EXTRAM_BASE;
            printdec(p[32]);   // word read
        } break;
        case TEST3: {
            uint32_t *p = (uint32_t *)UVM32_EXTRAM_BASE;
            p[32] = 1234;   // past the end, out of bounds
        } break;
        case TEST4: {
            uint32_t *p = (uint32_t *)UVM32_EXTRAM_BASE;
            p[0] = 1234; // word write
            yield(0);
        } break;
        case TEST5: {
            uint8_t *p = (uint8_t *)UVM32_EXTRAM_BASE;
            p[7] = 0xAB; // single byte write
            yield(0);
        } break;
        case TEST6: {
            uint8_t *p = (uint8_t *)UVM32_EXTRAM_BASE;
            printdec(p[7]); // single byte read 
        } break;
        case TEST7: {
            uint16_t *p = (uint16_t *)UVM32_EXTRAM_BASE;
            p[7] = 0xABCD; // short write
            yield(0);
        } break;
        case TEST8: {
            uint16_t *p = (uint16_t *)UVM32_EXTRAM_BASE;
            printdec(p[7]); // short read
        } break;
        case TEST9: {
            uint8_t *p = (uint8_t *)UVM32_EXTRAM_BASE;
            p[0] = 'h';
            p[1] = 'e';
            p[2] = 'l';
            p[3] = 'l';
            p[4] = 'o';
            printbuf(p, 5); // try to print from extram (unterminated)
        } break;
        case TEST10: {
            uint8_t *p = (uint8_t *)UVM32_EXTRAM_BASE;
            p[0] = 'h';
            p[1] = 'e';
            p[2] = 'l';
            p[3] = 'l';
            p[4] = 'o';
            p[5] = '\0';
            println(p); // try to print from extram (terminated)
        } break;

    }
}

