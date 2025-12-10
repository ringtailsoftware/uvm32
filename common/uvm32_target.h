// Common to all target code

#include "uvm32_sys.h"

// <stdint>
typedef unsigned long long uint64_t;
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef signed long long int64_t;
typedef signed long int32_t;
typedef signed short int16_t;
typedef signed char int8_t;

// <stdbool>
typedef unsigned char bool;
#define true 1
#define false 0

#ifndef size_assert
#define size_assert( what, howmuch ) \
  typedef char what##_size_wrong_[( !!(sizeof(what) == howmuch) )*2-1 ]
#endif

// sanity check
size_assert(uint64_t, 8);
size_assert(uint32_t, 4);
size_assert(uint16_t, 2);
size_assert(uint8_t, 1);
size_assert(int64_t, 8);
size_assert(int32_t, 4);
size_assert(int16_t, 2);
size_assert(int8_t, 1);

static uint32_t syscall(uint32_t id, uint32_t param1, uint32_t param2) {
    register uint32_t a0 asm("a0") = (uint32_t)(param1);
    register uint32_t a1 asm("a1") = (uint32_t)(param2);
    register uint32_t a2 asm("a2");
    register uint32_t a7 asm("a7") = (uint32_t)(id);

    asm volatile (
        "ecall"
        : "=r"(a2) // output
        : "r"(a7), "r"(a0), "r"(a1) // input
        : "memory"
    );
    return a2;
}

#define syscall_cast(id, p1, p2) syscall((uint32_t)id, (uint32_t)p1, (uint32_t)p2)

#define println(x)      syscall_cast(UVM32_SYSCALL_PRINTLN, x, 0)
#define print(x)        syscall_cast(UVM32_SYSCALL_PRINT, x, 0)
#define printdec(x)     syscall_cast(UVM32_SYSCALL_PRINTDEC, x, 0)
#define printhex(x)     syscall_cast(UVM32_SYSCALL_PRINTHEX, x, 0)
#define millis()        syscall_cast(UVM32_SYSCALL_MILLIS, 0, 0)
#define putc(x)         syscall_cast(UVM32_SYSCALL_PUTC, x, 0)
#define getc()          syscall_cast(UVM32_SYSCALL_GETC, 0, 0)
#define yield()         syscall_cast(UVM32_SYSCALL_YIELD, 0, 0)
#define printbuf(x, y)  syscall_cast(UVM32_SYSCALL_PRINTBUF, x, y)

extern char _estack;

static void stackprotect(void) {
    syscall_cast(UVM32_SYSCALL_STACKPROTECT, &_estack, 0);
}

#include "uvm32_common_custom.h"

