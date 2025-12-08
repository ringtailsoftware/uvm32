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

pub inline fn getc() ?u8 {
    const key = syscall(uvm32.UVM32_SYSCALL_GETC, 0);
    if (key == 0xFFFFFFFF) {
        return null;
    } else {
        return @truncate(key);
    }
}

pub inline fn millis() u32 {
    return syscall(uvm32.UVM32_SYSCALL_MILLIS, 0);
}

// dupeZ would be better, but want to avoid using an allocator
// this is of course, unsafe...
var termination_buf:[128]u8 = undefined;

pub inline fn print(m: []const u8) void {
    @memcpy(termination_buf[0..m.len], m);
    termination_buf[m.len] = 0;
    const s = termination_buf[0..m.len :0];
    _ = syscall(uvm32.UVM32_SYSCALL_PRINT, @intFromPtr(s.ptr));
}

pub inline fn println(m: []const u8) void {
    @memcpy(termination_buf[0..m.len], m);
    termination_buf[m.len] = 0;
    const s = termination_buf[0..m.len :0];
    _ = syscall(uvm32.UVM32_SYSCALL_PRINTLN, @intFromPtr(s.ptr));
}

pub inline fn yield() void {
    _ = syscall(uvm32.UVM32_SYSCALL_YIELD, 0);
}

pub inline fn putc(c:u8) void {
    _ = syscall(uvm32.UVM32_SYSCALL_PUTC, c);
}

