load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "test_sink",
    testonly = 1,
    srcs = ["test_sink.cc"],
    hdrs = ["test_sink.h"],
    deps = ["//nsasm:memory"],
)

cc_library(
    name = "test_assembly",
    testonly = 1,
    srcs = ["test_assembly.cc"],
    hdrs = ["test_assembly.h"],
    deps = [
        ":test_sink",
        "//nsasm:assembler",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "test_rom",
    testonly = 1,
    srcs = ["test_rom.cc"],
    hdrs = ["test_rom.h"],
    deps = [
        "//nsasm:assembler",
        "//nsasm:error",
        "//nsasm:rom",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "test_disassembly",
    testonly = 1,
    srcs = ["test_disassembly.cc"],
    hdrs = ["test_disassembly.h"],
    deps = [
        "//nsasm:calling_convention",
        "//nsasm:disassemble",
        "//nsasm:error",
        "//nsasm:listing",
        "//nsasm:memory",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "simple_tests",
    srcs = ["simple_tests.cc"],
    deps = [
        ":test_assembly",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "cross_module_dependencies_test",
    srcs = ["cross_module_dependencies_test.cc"],
    deps = [
        ":test_assembly",
        ":test_sink",
        "@googletest//:gtest_main",
    ],
)
