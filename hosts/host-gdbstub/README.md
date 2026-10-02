# Why?

To debug your guest application. GDB can attach to the host over TCP socket, stdio,
serial connection, etc. In the provided example, it communicate over TCP socket.

# How to use it

1. Compile a guest application, for example from [apps](../../apps/) directory.
   To have a better debugging experience, use `-O0` and `-g3`.

2. Run the host prividing the app binary file. Use the following command to
   redirect stdio to a TCP socket:

```
./host-gdbstub ../../apps/maze/maze.bin"
```

3. Fire up a `riscv32-elf-gdb` (might be different name on your OS) and give
   it the generated elf file from step 1:

`riscv32-elf-gdb -ex 'target remote :1234' -ex 'layout src' maze.elf`

4. Happy debugging. `s`, `si`, `n`, breakpoint, etc should be working :)
