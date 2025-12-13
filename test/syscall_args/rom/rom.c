#include "uvm32_target.h"
#include "../shared.h"

void syscall_a_test(void) {
    // two strings
    syscall((uint32_t)SYSCALL_A, (uint32_t)SYSCALL_A_DATA0, (uint32_t)SYSCALL_A_DATA1);
}

void syscall_b_test(void) {
    // two null values
    syscall((uint32_t)SYSCALL_B, 0, 0);
}

void syscall_c_test(void) {
    // valid buffer
    uint8_t buf[32];
    for (int i=0;i<32;i++) {
        buf[i] = i;
    }
    syscall((uint32_t)SYSCALL_C, (uint32_t)buf, sizeof(buf));
}

void syscall_d_test(void) {
    // invalid address
    syscall((uint32_t)SYSCALL_D, 0xFF000000, 100);
}

void syscall_ef_test(void) {
    // ask for two values, then send them back
    uint32_t a, b;
    syscall((uint32_t)SYSCALL_E, (uint32_t)&a, (uint32_t)&b);
    syscall((uint32_t)SYSCALL_F, a, b);
}

void syscall_gh_test(void) {
    // ask for a buffer, mutate it, send it back
    uint8_t buf[32];
    syscall((uint32_t)SYSCALL_G, (uint32_t)buf, 32);
    for (int i=0;i<32;i++) {
        buf[i] *= 2;
    }
    syscall((uint32_t)SYSCALL_H, (uint32_t)buf, 32);
}

void syscall_i_test(void) {
    char *p = "hello";  // runner will overwrite memory
    syscall((uint32_t)SYSCALL_I, (uint32_t)p, 0);
}

void main(void) {
    switch(syscall(SYSCALL_PICKTEST, 0, 0)) {
        case SYSCALL_A:
            syscall_a_test();
        break;
        case SYSCALL_B:
            syscall_b_test();
        break;
        case SYSCALL_C:
            syscall_c_test();
        break;
        case SYSCALL_D:
            syscall_d_test();
        break;
        case SYSCALL_E:
            syscall_ef_test();
        break;
        case SYSCALL_G:
            syscall_gh_test();
        break;
        case SYSCALL_I:
            syscall_i_test();
        break;

    }
}

