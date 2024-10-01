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

cc_test(
    name = "simple_tests",
    srcs = ["simple_tests.cc"],
    deps = [
        ":test_assembly",
        "@googletest//:gtest_main",
    ],
)
