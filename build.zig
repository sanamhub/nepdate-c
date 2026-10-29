const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const core = b.addLibrary(.{
        .name = "nepdate-c",
        .linkage = .static,
        .root_module = b.createModule(.{ .target = target, .optimize = optimize }),
    });
    core.root_module.addCSourceFiles(.{
        .files = &.{ "src/nepdate.c", "src/tm.c" },
        .flags = &.{"-std=c11"},
    });
    core.root_module.addIncludePath(b.path("include"));
    core.root_module.addIncludePath(b.path("src"));
    // tm.c needs <time.h>; the core does not call the C library, but the header is a libc header.
    core.root_module.link_libc = true;
    core.installHeader(b.path("include/nepdate.h"), "nepdate.h");
    b.installArtifact(core);

    const patro = b.addLibrary(.{
        .name = "nepdate-c-patro",
        .linkage = .static,
        .root_module = b.createModule(.{ .target = target, .optimize = optimize }),
    });
    patro.root_module.addCSourceFiles(.{
        .files = &.{"src/patro.c"},
        .flags = &.{"-std=c11"},
    });
    patro.root_module.addIncludePath(b.path("include"));
    patro.root_module.addIncludePath(b.path("src"));
    patro.root_module.linkLibrary(core);
    patro.installHeader(b.path("include/nepdate_patro.h"), "nepdate_patro.h");
    b.installArtifact(patro);
}
