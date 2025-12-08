const uvm = @import("uvm.zig");

fn mandel() void {
    const xmin: i32 = -8601;
    const xmax: i32 = 2867;
    const ymin: i32 = -4915;
    const ymax: i32 = 4915;
    const maxiter: usize = 32;
    const dx: i32 = @divTrunc((xmax - xmin), 79);
    const dy: i32 = @divTrunc((ymax - ymin), 24);
    var cy = ymin;

    while (cy <= ymax) {
        var cx = xmin;
        while (cx <= xmax) {
            var x: i32 = 0;
            var y: i32 = 0;
            var x2: i32 = 0;
            var y2: i32 = 0;
            var iter: usize = 0;
            while (iter < maxiter) : (iter += 1) {
                if (x2 + y2 > 16384) break;
                y = ((x * y) >> 11) + cy;
                x = x2 - y2 + cx;
                x2 = (x * x) >> 12;
                y2 = (y * y) >> 12;
                uvm.yield();
            }
            uvm.putc(' ' + @as(u8, @intCast(iter)));
            cx += dx;
        }
        uvm.putc('\n');
        cy += dy;
    }
}

export fn main() void {
    mandel();
    uvm.println("Hello world");
}
