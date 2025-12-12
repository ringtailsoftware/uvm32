# 🌱 uvm32

uvm32 is a minimalist, dependency-free virtual machine sandbox designed for microcontrollers and other resource-constrained devices. Single C file, no dynamic memory allocations, asynchronous design, pure C99.

On an [STM32L0](https://www.st.com/en/microcontrollers-microprocessors/stm32l0-series.html) (ARM Cortex-M0+) the required footprint is under 4KB flash/1KB RAM.

## What is it for?

* As a no-frills alternative to embedded script engines ([Lua](https://www.lua.org/), [Duktape](https://duktape.org/), [MicroPython](https://micropython.org/), etc)
* As a [sandbox](https://en.wikipedia.org/wiki/Write_once,_run_anywhere) to isolate untrusted or unreliable elements of a system
* As a way to allow development in modern systems programming languages where a compiler for the target may not be available ([rust-hello](apps/rust-hello))
* As a way to [write once, run anywhere](https://en.wikipedia.org/wiki/Write_once,_run_anywhere) and avoid maintaining multiple software variants

## Features

* Bytecode example apps written in C, Zig, Rust and assembly
* Non-blocking design, preventing misbehaving bytecode from stalling the host
* No assumptions about host IO capabilities (no stdio)
* Simple, opinionated execution model
* Safe minimally typed FFI
* Small enough for "if this then that" scripts/plugins, capable enough for [much more](apps/zigdoom)
* Aims for safety over speed, bad code running in the VM should never be able to crash the host

Although based on a [fully fledged CPU emulator](https://github.com/cnlohr/mini-rv32ima), uvm32 is intended for executing custom script like logic, not for simulating hardware.

## Understanding this repo

uvm32 is a tiny virtual machine, all of the code is in [uvm32](uvm32).

A minimal example of a host to run code in is at [host-mini](hosts/host-mini).

Everything else is a more advanced host, or a sample application which could be run.

## Example

A simple VM host from [host-mini](hosts/host-mini)

```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uvm32.h"
#include "uvm32_common_custom.h"

uint8_t rom[] = { // mandel.bin
  0x23, 0x26, 0x11, 0x00, 0xef, 0x00, 0xc0, 0x00, 0xb7, 0x08, 0x00, 0x01,
  ...
  ...
};

int main(int argc, char *argv[]) {
    uvm32_state_t vmst;
    uvm32_evt_t evt;
    bool isrunning = true;

    uvm32_init(&vmst);
    uvm32_load(&vmst, rom, sizeof(rom));

    while(isrunning) {
        uvm32_run(&vmst, &evt, 100);   // num instructions before vm considered hung

        switch(evt.typ) {
            case UVM32_EVT_END:
                isrunning = false;
            break;
            case UVM32_EVT_SYSCALL:    // vm has paused to handle UVM32_SYSCALL
                switch(evt.data.syscall.code) {
                    case UVM32_SYSCALL_PUTC:
                        printf("%c", uvm32_getval(&vmst, &evt, ARG0));
                    break;
                    case UVM32_SYSCALL_PRINTLN: {
                        const char *str = uvm32_getcstr(&vmst, &evt, ARG0);
                        printf("%s\n", str);
                    } break;
                    case UVM32_SYSCALL_YIELD:
                    break;
                    default:
                        printf("Unhandled syscall 0x%08x\n", evt.data.syscall.code);
                    break;
                }
            break;
            case UVM32_EVT_ERR:
                printf("UVM32_EVT_ERR '%s' (%d)\n", evt.data.err.errstr, (int)evt.data.err.errcode);
            break;
            default:
            break;
        }
    }

    return 0;
}
```

## Samples

 * [host](hosts/host) vm host which loads a binary and runs to completion, handling multiple syscall types
 * [host-mini](hosts/host-mini) minimal vm host (shown above), with baked in bytecode
 * [host-parallel](hosts/host-parallel) parallel vm host running multiple vm instances concurrently, with baked in bytecode
 * [host-arduino](hosts/host-arduino) vm host as Arduino sketch (`make test` to run AVR code in qemu)
 * [apps/helloworld](apps/helloworld) C hello world program
 * [apps/conio](apps/conio) C console IO demo
 * [apps/lissajous](apps/lissajous) C console lissajous curve (showing softfp, floating point)
 * [apps/maze](apps/maze) C ASCII art recursive maze generation
 * [apps/hello-asm](apps/hello-asm) Minimal hello world assembly
 * [apps/fib](apps/fib) C fibonacci series program (iterative and recursive)
 * [apps/self](apps/self) host-mini with embedded mandelbrot generation program, compiled as an app (inception!)
 * [apps/sketch](apps/sketch) C Arduino/Wiring/Processing type program in `setup()` and `loop()` style
 * [apps/rust-hello](apps/rust-hello) Rust hello world program (note, the version of rust installed by brew on mac has issues, use the official rust installer from https://rust-lang.org/learn/get-started/)
 * [apps/zig-mandel](apps/zig-mandel) Zig ASCII mandelbrot generator program
 * [apps/zigtris](apps/zigtris) Zig Tetris (https://github.com/ringtailsoftware/zigtris)
 * [apps/zigalloc](apps/zigalloc) Demonstration of using extram with zig allocator
 * [apps/heap](apps/heap) Demonstration of `malloc()` on extram in C
 * [apps/zigdoom](apps/zigdoom) Port of PureDOOM (making use of Zig to provide an allocator and libc like functions)

## Quickstart (docker)

    make dockerbuild
    make dockershell

Then, from inside the docker shell

    make

    ./hosts/host/host apps/helloworld/helloworld.bin

`host` is the command line test VM for running samples. Run `host -h` for a full list of options.

## More information

Start at [doc/README.md](doc/README.md)

## License

This project is licensed under the MIT License. Feel free to use in research, products and embedded devices.
