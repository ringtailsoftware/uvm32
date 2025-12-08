# 🧮 uvm32

uvm32 is a minimalist, dependency-free virtual machine sandbox designed for microcontrollers and other resource-constrained devices. Single C file, no dynamic memory allocations, asynchronous design, pure C99.

## Features

* Bytecode example apps written in C, Zig, Rust and assembly
* Non-blocking design, preventing misbehaving bytecode from stalling the host
* No assumptions about host IO capabilities (no stdio)
* Simple, opinionated execution model
* Safe minimally typed FFI
* Small enough for "if this then that" scripts/plugins, capable enough for much more

Although based on a fully fledged CPU emulator, uvm32 is intended for executing custom script like logic, not for simulating hardware.

## Samples

 * [host](host) vm host which loads a binary and runs to completion, handling multiple syscall types
 * [host-mini](host-mini) minimal vm host (shown above), with baked in bytecode
 * [host-parallel](host-parallel) parallel vm host running multiple vm instances concurrently, with baked in bytecode
 * [host-arduino](host-arduino) vm host as Arduino sketch (tested on Arduino Uno ATmega328P, uses 9950 bytes of flash/1254 bytes RAM)
 * [apps/helloworld](apps/helloworld) C hello world program
 * [apps/conio](apps/conio) C console IO demo
 * [apps/lissajous](apps/lissajous) C console lissajous curve (showing softfp, floating point)
 * [apps/hello-asm](apps/hello-asm) Minimal hello world assembly
 * [apps/fib](apps/fib) C fibonacci series program (iterative and recursive)
 * [apps/sketch](apps/sketch) C Arduino/Wiring/Processing type program in `setup()` and `loop()` style
 * [apps/rust-hello](apps/rust-hello) Rust hello world program (note, the version of rust installed by brew on mac has issues, use the official rust installer from https://rust-lang.org/learn/get-started/)
 * [apps/zig-mandel](apps/zig-mandel) Zig ASCII mandelbrot generator program
 * [apps/zigtris](apps/zigtris) Zig Tetris (https://github.com/ringtailsoftware/zigtris) WASD+space to play

## Quickstart

    make
    host/host precompiled/mandel.bin
    host/host precompiled/zigtris.bin

Build sample apps (sets up docker for cross compiler)

    cd apps
    make

Build one of the sample apps (requires docker for C, or Zig, or Rust)

	cd apps/helloworld && make
	
Run the app

	./host ../apps/helloworld/helloworld.bin

## Quickstart API

```c
uint8_t bytecode[] = { /* ... */ }; // some compiled bytecode
uvm32_state_t vmst; // execution state of the vm
uvm32_evt_t evt; // events passed from vm to host

uvm32_init(&vmst, NULL, 0); // setup vm and pass in handlers for host functions
uvm32_load(&vmst, bytecode, sizeof(bytecode)); // load the bytecode
uvm32_run(&vmst, &evt, 100); // run up to 100 instructions

switch(evt.typ) {
	// check why the vm stopped executing
}
```

## Operation

Once loaded with bytecode, uvm32's state is advanced by calling `uvm32_run()`.

	uint32_t uvm32_run(uvm32_state_t *vmst, uvm32_evt_t *evt, uint32_t instr_meter)
	
`uvm32_run()` will execute until the bytecode requests some IO activity from the host.
These IO activities are called "syscalls" and are the only way for bytecode to communicate with the host.
If the bytecode attempts to execute more instructions than the the passed value of `instr_meter` it is assumed to have crashed and an error is reported.

(As with a watchdog on an embedded system, the `yield()` bytecode function tells the host that the code requires more time to complete and has not hung)

`uvm32_run()` always returns an event. There are four possible events:

* `UVM32_EVT_END` the program has ended
* `UVM32_EVT_ERR` the program has encountered an error
* `UVM32_EVT_YIELD` the program has called `yield()` signifying that it requires more instructions to be executed, but has not crashed/hung
* `UVM32_EVT_SYSCALL` the program requests some IO via the host

## Internals

uvm32 emulates a RISC-V 32bit CPU using [mini-rv32ima](https://github.com/cnlohr/mini-rv32ima). All IO from vm bytecode to the host is performed using `ecall` syscalls. Each syscall provided by the host requires a unique syscall value. A syscall passes a single `uint32_t` from bytecode to the host and may receive a returned `uint32_t`. The host may treat the value as a pointer and modify memory.

uvm32 is always in one of 4 states, paused, running, ended or error.

```mermaid
stateDiagram
    [*] --> UVM32_STATUS_PAUSED : uvm32_init()
    UVM32_STATUS_PAUSED-->UVM32_STATUS_RUNNING : uvm32_run()
    UVM32_STATUS_RUNNING --> UVM32_STATUS_PAUSED : syscall event
    UVM32_STATUS_RUNNING --> UVM32_STATUS_ENDED : halt()
    UVM32_STATUS_RUNNING --> UVM32_STATUS_ERROR
```

## Boot

At boot, the whole memory is zeroed. The user program is placed at the start, the CPU registers are stored at the end. The stack pointer is set to the start of the CPU registers and grows downwards.

## syscall ABI

All communication between bytecode and the vm host is performed via syscalls.

To make a syscall, register `a7` is set with the syscall number (a `UVM32_SYSCALL_x`) and `a0` is set with the syscall parameter. The response is returned in `a1`.

[target.h](common/uvm32_target.h#L12)

```c
static uint32_t syscall(uint32_t id, uint32_t param) {
    register uint32_t a0 asm("a0") = (uint32_t)(param);
    register uint32_t a1 asm("a1");
    register uint32_t a7 asm("a7") = (uint32_t)(id);

    asm volatile (
        "ecall"
        : "=r"(a1) // output
        : "r"(a0), "r"(a7) // input
        : "memory"
    );
    return a1;
}
```

The [RISC-V SBI](https://github.com/riscv-non-isa/riscv-sbi-doc/blob/master/riscv-sbi.adoc) is not followed, a simpler approach is taken.

## syscalls

There are two system syscalls used by uvm32, `halt()` and `yield()`.

`halt()` tells the host that the program has ended normally. `yield()` tells the host that the program requires more instructions to be executed.

New syscalls can be added to the host via `uvm32_init()`.
Each syscall maps a syscall number to a value understood by the host (`F_PRINTD` below) and has an associated type which tells the host how to interpret the data passed to the syscall.

Here is a full example of a working VM host from [apps/host-mini](apps/host-mini)

--

```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uvm32.h"
#include "../common/uvm32_common_custom.h"

// Precompiled binary program to print integers
// This code expects to print via syscall 0x13C (UVM32_SYSCALL_PRINTD in common/uvm32_common_custom.h)
uint8_t rom[] = {
  0x23, 0x26, 0x11, 0x00, 0xef, 0x00, 0x00, 0x01, 0x73, 0x50, 0x80, 0x13,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x93, 0x07, 0x00, 0x00,
  0x13, 0x07, 0xa0, 0x00, 0x73, 0x90, 0xc7, 0x13, 0x93, 0x87, 0x17, 0x00,
  0xe3, 0x9c, 0xe7, 0xfe, 0x67, 0x80, 0x00, 0x00
};

// Create an identifier for our host handler
typedef enum {
    F_PRINTD,
} f_code_t;

// Map VM syscall UVM32_SYSCALL_PRINTD (0x13C) to F_PRINTD, tell VM to expect write of a U32
const uvm32_mapping_t env[] = {
    { .syscall = UVM32_SYSCALL_PRINTD, .typ = UVM32_SYSCALL_TYP_U32_WR, .code = F_PRINTD },
};

int main(int argc, char *argv[]) {
    uvm32_state_t vmst;
    uvm32_evt_t evt;
    bool isrunning = true;

    uvm32_init(&vmst, env, sizeof(env) / sizeof(env[0]));
    uvm32_load(&vmst, rom, sizeof(rom));

    while(isrunning) {
        uvm32_run(&vmst, &evt, 100);   // num instructions before vm considered hung

        switch(evt.typ) {
            case UVM32_EVT_END:
                isrunning = false;
            break;
            case UVM32_EVT_SYSCALL:    // vm has paused to handle UVM32_SYSCALL
                switch((f_code_t)evt.data.syscall.code) {
                    case F_PRINTD:
                        // Type of F_PRINTD is UVM32_SYSCALL_TYP_U32_WR, so expect value in evt.data.syscall.val.u32
                        printf("%d\n", evt.data.syscall.val.u32);
                    break;
                }
            break;
            default:
            break;
        }
    }

    return 0;
}
```

## Configuration

The uvm32 memory size is set at compile time with `-DUVM32_MEMORY_SIZE=X` (in bytes). A memory of 512 bytes will be sufficient for trivial programs.

## Debugging

The `Makefile` in `apps` builds a docker container with a RISC-V toolchain. This can be used to disassemble with:

    docker run -v `pwd`:/data -w /data --rm riscv-dev riscv64-unknown-elf-objdump -d -f hello-asm.elf
    docker run -v `pwd`:/data -w /data --rm riscv-dev riscv64-unknown-elf-objdump -S -d -b binary -m riscv:rv32 -D -M no-aliases -f hello-asm.bin

## License

This project is licensed under the MIT License. Feel free to use in research, products and embedded devices.
