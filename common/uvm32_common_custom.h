// Definitions needed by both host and target

// syscalls for exposed host functions, start at 0
#define UVM32_SYSCALL_PUTC      0x00000000
#define UVM32_SYSCALL_GETC      0x00000001
#define UVM32_SYSCALL_PRINT     0x00000002
#define UVM32_SYSCALL_PRINTLN   0x00000003
#define UVM32_SYSCALL_PRINTDEC  0x00000004
#define UVM32_SYSCALL_PRINTHEX  0x00000005
#define UVM32_SYSCALL_MILLIS    0x00000006

