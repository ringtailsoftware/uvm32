const std = @import("std");
const CrossTarget = @import("std").zig.CrossTarget;
const Target = @import("std").Target;
const Feature = @import("std").Target.Cpu.Feature;

pub fn build(b: *std.Build) void {
    var options = b.addOptions();
    const heapsize = b.option(u32, "heapsize", "heap size in bytes") orelse 0;   // -Dheapsize=u32
    options.addOption(u32, "heapsize", heapsize);

    const features = Target.riscv.Feature;
    var disabled_features = Feature.Set.empty;
    var enabled_features = Feature.Set.empty;

    // disable all CPU extensions
    disabled_features.addFeature(@intFromEnum(features.a));
//    disabled_features.addFeature(@intFromEnum(features.c));
    disabled_features.addFeature(@intFromEnum(features.d));
    disabled_features.addFeature(@intFromEnum(features.e));
    disabled_features.addFeature(@intFromEnum(features.f));
    // except multiply
    enabled_features.addFeature(@intFromEnum(features.m));
    enabled_features.addFeature(@intFromEnum(features.c));

    const target = b.resolveTargetQuery(.{
        .cpu_arch = Target.Cpu.Arch.riscv32,
        .os_tag = Target.Os.Tag.freestanding,
        .abi = Target.Abi.none,
        .cpu_model = .{ .explicit = &std.Target.riscv.cpu.generic_rv32},
        .cpu_features_sub = disabled_features,
        .cpu_features_add = enabled_features
    });

    const exe = b.addExecutable(.{
        .name = "zigdoom",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = .ReleaseSmall,
        }),
    });
    exe.root_module.addOptions("buildopts", options);

    // add zeptolibc
    const zeptolibc_dep = b.dependency("zeptolibc", .{
        .target = target,
        .optimize = .ReleaseSmall,
    });
    exe.root_module.addImport("zeptolibc", zeptolibc_dep.module("zeptolibc"));
    exe.root_module.addIncludePath(zeptolibc_dep.path("include"));
    exe.root_module.addIncludePath(zeptolibc_dep.path("include/zeptolibc"));

    exe.addCSourceFiles(.{ 
        .files = &.{
            "src/puredoom/DOOM.c",     "src/puredoom/PureDOOM.c", "src/puredoom/am_map.c",
            "src/puredoom/d_items.c",  "src/puredoom/d_main.c",   "src/puredoom/d_net.c",
            "src/puredoom/doomdef.c",  "src/puredoom/doomstat.c", "src/puredoom/dstrings.c",
            "src/puredoom/f_finale.c", "src/puredoom/f_wipe.c",   "src/puredoom/g_game.c",
            "src/puredoom/hu_lib.c",   "src/puredoom/hu_stuff.c", "src/puredoom/i_net.c",
            "src/puredoom/i_sound.c",  "src/puredoom/i_system.c", "src/puredoom/i_video.c",
            "src/puredoom/info.c",     "src/puredoom/m_argv.c",   "src/puredoom/m_bbox.c",
            "src/puredoom/m_cheat.c",  "src/puredoom/m_fixed.c",  "src/puredoom/m_menu.c",
            "src/puredoom/m_misc.c",   "src/puredoom/m_random.c", "src/puredoom/m_swap.c",
            "src/puredoom/p_ceilng.c", "src/puredoom/p_doors.c",  "src/puredoom/p_enemy.c",
            "src/puredoom/p_floor.c",  "src/puredoom/p_inter.c",  "src/puredoom/p_lights.c",
            "src/puredoom/p_map.c",    "src/puredoom/p_maputl.c", "src/puredoom/p_mobj.c",
            "src/puredoom/p_plats.c",  "src/puredoom/p_pspr.c",   "src/puredoom/p_saveg.c",
            "src/puredoom/p_setup.c",  "src/puredoom/p_sight.c",  "src/puredoom/p_spec.c",
            "src/puredoom/p_switch.c", "src/puredoom/p_telept.c", "src/puredoom/p_tick.c",
            "src/puredoom/p_user.c",   "src/puredoom/r_bsp.c",    "src/puredoom/r_data.c",
            "src/puredoom/r_draw.c",   "src/puredoom/r_main.c",   "src/puredoom/r_plane.c",
            "src/puredoom/r_segs.c",   "src/puredoom/r_sky.c",    "src/puredoom/r_things.c",
            "src/puredoom/s_sound.c",  "src/puredoom/sounds.c",   "src/puredoom/st_lib.c",
            "src/puredoom/st_stuff.c", "src/puredoom/tables.c",   "src/puredoom/v_video.c",
            "src/puredoom/w_wad.c",    "src/puredoom/wi_stuff.c", "src/puredoom/z_zone.c",
        },
        .flags = &.{ "-Wall", "-fno-sanitize=undefined" }
    });
    exe.addIncludePath(b.path("src/"));

    b.installArtifact(exe);

    exe.addAssemblyFile(b.path("../common/crt0.S"));
    exe.setLinkerScript(b.path("../common/linker.ld"));
    exe.addIncludePath(b.path("../../common"));
    exe.addIncludePath(b.path("../common"));

    const bin = b.addObjCopy(exe.getEmittedBin(), .{
        .format = .bin,
    });
    bin.step.dependOn(&exe.step);

    const copy_bin = b.addInstallBinFile(bin.getOutput(), "zigdoom.bin");
    b.default_step.dependOn(&copy_bin.step);
}
