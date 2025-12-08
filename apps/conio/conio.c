#define USE_MAIN
#include "uvm32_target.h"

void main(void) {
    uint32_t c;
    println("Press a key!");
    while(c = getc()) {
        if (c != 0xFFFFFFFF) {
            print("Got: ");
            printhex(c);
            println("");
        }
    }
}

