#![no_std]
#![no_main]

use core::arch::global_asm;
use core::arch::asm;
use core::panic::PanicInfo;

// fetch UVM32_SYSCALL definitions from C header
include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

// startup code
global_asm!(include_str!("../../crt0.S"));

fn syscall(id: u32, param1: u32, param2: u32) -> u32 {
    let mut value;
    unsafe {
        asm!("ecall",
            in("a0") param1, in("a1") param2,
            in("a7") id,
            lateout("a2") value,
        );
    }
    return value;
}

fn println(message: &str) {
    let addr_value = message.as_ptr() as u32;
    syscall(UVM32_SYSCALL_PRINTLN, addr_value, 0);
}

fn printdec(n: u32) {
    syscall(UVM32_SYSCALL_PRINTDEC, n, 0);
}

#[no_mangle]
pub extern "C" fn main() {
    for i in 0..10 {
        printdec(i);
        println("\0");
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
