#include "uvm32_target.h"

uint32_t count;

bool loop(void) {
    printdec(count);
    if (count++ >= 10) {
        return false;
    } else {
        return true;
    }
}

void setup(void) {
    count = 0;
}


