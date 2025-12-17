const std = @import("std");
const CrossTarget = @import("std").zig.CrossTarget;
const Target = @import("std").Target;
const Feature = @import("std").Target.Cpu.Feature;

pub fn build(b: *std.Build) void {
    var options = b.addOptions();
    const heapsize = b.option(u32, "heapsize", "heap size in bytes") orelse 0; // -Dheapsize=u32
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

    const target = b.resolveTargetQuery(.{ .cpu_arch = Target.Cpu.Arch.riscv32, .os_tag = Target.Os.Tag.freestanding, .abi = Target.Abi.none, .cpu_model = .{ .explicit = &std.Target.riscv.cpu.generic_rv32 }, .cpu_features_sub = disabled_features, .cpu_features_add = enabled_features });

    const exe = b.addExecutable(.{
        .name = "tinygl",
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

    exe.addCSourceFiles(.{ .files = &.{
        "src/TinyGL/src/api.c",
        "src/TinyGL/src/specbuf.c",
        "src/TinyGL/src/zmath.c",
        "src/TinyGL/src/arrays.c",
        "src/TinyGL/src/image_util.c",
        "src/TinyGL/src/misc.c",
        "src/TinyGL/src/texture.c",
        "src/TinyGL/src/ztriangle.c",
        "src/TinyGL/src/clear.c",
        "src/TinyGL/src/init.c",
        "src/TinyGL/src/msghandling.c",
        "src/TinyGL/src/vertex.c",
        "src/TinyGL/src/clip.c",
        "src/TinyGL/src/light.c",
        "src/TinyGL/src/zbuffer.c",
        "src/TinyGL/src/error.c",
        "src/TinyGL/src/list.c",
        "src/TinyGL/src/zdither.c",
        "src/TinyGL/src/get.c",
        "src/TinyGL/src/matrix.c",
        "src/TinyGL/src/select.c",
        "src/TinyGL/src/zline.c",
    }, .flags = &.{ "-Wall", "-fno-sanitize=undefined" } });
    exe.addIncludePath(b.path("src/"));
    exe.addIncludePath(b.path("src/TinyGL/include"));
    exe.addIncludePath(b.path("src/TinyGL/src"));

    b.installArtifact(exe);

    exe.addAssemblyFile(b.path("../common/crt0.S"));
    exe.setLinkerScript(b.path("../common/linker.ld"));
    exe.addIncludePath(b.path("../../common"));
    exe.addIncludePath(b.path("../common"));

    const bin = b.addObjCopy(exe.getEmittedBin(), .{
        .format = .bin,
    });
    bin.step.dependOn(&exe.step);

    const copy_bin = b.addInstallBinFile(bin.getOutput(), "tinygl.bin");
    b.default_step.dependOn(&copy_bin.step);
}
