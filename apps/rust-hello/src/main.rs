#![no_std]
#![no_main]

use core::arch::global_asm;
use core::arch::asm;
use core::panic::PanicInfo;

// fetch UVM32_SYSCALL definitions from C header
include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

// startup code
global_asm!(include_str!("../../crt0.S"), UVM32_SYSCALL_HALT = const UVM32_SYSCALL_HALT);

fn syscall(id: u32, n: u32) -> u32 {
    let mut value;
    unsafe {
        asm!("ecall",
            in("a0") n,
            in("a7") id,
            lateout("a1") value,
        );
    }
    return value;
}

fn println(message: &str) {
    let addr_value = message.as_ptr() as u32;
    syscall(UVM32_SYSCALL_PRINTLN, addr_value);
}

fn printdec(n: u32) {
    syscall(UVM32_SYSCALL_PRINTDEC, n);
}

#[no_mangle]
pub extern "C" fn main() {
    for i in 0..10 {
        printdec(i);
    }
    println("Hello, world!\0");
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    //println("Something went wrong");
    loop {
        continue;
    }
}
