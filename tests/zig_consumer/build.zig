const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const nepdate = b.dependency("nepdate_c", .{ .target = target, .optimize = optimize });

    const translate = b.addTranslateC(.{
        .root_source_file = b.path("src/c.h"),
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    translate.addIncludePath(nepdate.path("include"));

    const exe = b.addExecutable(.{
        .name = "nepdate-zig-consumer",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .imports = &.{.{ .name = "c", .module = translate.createModule() }},
        }),
    });
    exe.root_module.linkLibrary(nepdate.artifact("nepdate-c"));
    exe.root_module.linkLibrary(nepdate.artifact("nepdate-c-patro"));
    b.installArtifact(exe);

    const run = b.step("run", "Run the consumer");
    const run_exe = b.addRunArtifact(exe);
    run_exe.step.dependOn(b.getInstallStep());
    run.dependOn(&run_exe.step);
}
