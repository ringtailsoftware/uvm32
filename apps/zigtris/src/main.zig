const uvm = @import("uvm.zig");

const mibu = @import("mibu");
const zigtris = @import("zigtris");
var nextEvent:mibu.events.Event = .none;

const console = @import("console.zig").getWriter();

export fn main() void {
    var gameRunning = true;
    var lastUpdate:u32 = 0;

    zigtris.gamesetup(console, uvm.millis()) catch |err| {
        _ = console.print("err {any}\n", .{err}) catch 0;
        _ = console.flush() catch 0;
        return;
    };

    while(gameRunning) {
        const now = uvm.millis();
        if (uvm.getc()) |key| {
            switch(key) {
                ' ' => nextEvent = mibu.events.Event{.key = .{.code = .{.char = ' '}}},
                'a' => nextEvent = mibu.events.Event{.key = .{.code = .left}},
                'd' => nextEvent = mibu.events.Event{.key = .{.code = .right}},
                'w' => nextEvent = mibu.events.Event{.key = .{.code = .up}},
                's' => nextEvent = mibu.events.Event{.key = .{.code = .down}},
                'q' => gameRunning = false,
                else => {},
            }
        }

        if (now > lastUpdate + 100 or nextEvent != .none) {

            lastUpdate = now;
            gameRunning = zigtris.gameloop(console, now, nextEvent) catch false;
            nextEvent = .none;
            _ = console.flush() catch 0;
        }
    }
}
