const uvm32 = @cImport({
    @cDefine("USE_MAIN", "1");
    @cInclude("uvm32_target.h");
});
const std = @import("std");

pub inline fn syscall(id: u32, param: u32) u32 {
    var val: u32 = undefined;
    asm volatile ("ecall"
        : [val] "={a1}" (val),
        : [param] "{a0}" (param),
          [id] "{a7}" (id),
        : .{ .memory = true });
    return val;
}

pub inline fn println(val: [:0]const u8) void {
    _ = syscall(uvm32.UVM32_SYSCALL_PRINTLN, @intFromPtr(val.ptr));
}

pub inline fn yield() void {
    _ = syscall(uvm32.UVM32_SYSCALL_YIELD, 0);
}

pub inline fn putc(c:u8) void {
    _ = syscall(uvm32.UVM32_SYSCALL_PUTC, c);
}

