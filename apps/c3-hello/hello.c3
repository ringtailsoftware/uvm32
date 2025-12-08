//static uint32_t syscall(uint32_t id, uint32_t param) {
//    register uint32_t a0 asm("a0") = (uint32_t)(param);
//    register uint32_t a1 asm("a1");
//    register uint32_t a7 asm("a7") = (uint32_t)(id);
//
//    asm volatile (
//        "ecall"
//        : "=r"(a1) // output
//        : "r"(a0), "r"(a7) // input
//        : "memory"
//    );
//    return a1;
//}

fn void main() @export("main") {
    int x = 42;
    TBD
}

