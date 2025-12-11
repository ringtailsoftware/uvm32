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
            printdec(p[32]);   // past the end
        } break;
        case TEST3: {
            uint32_t *p = (uint32_t *)UVM32_EXTRAM_BASE;
            p[32] = 1234;   // past the end
        } break;
        case TEST4: {
            uint32_t *p = (uint32_t *)UVM32_EXTRAM_BASE;
            p[0] = 1234; // good write
            yield(0);
        } break;

    }

}

