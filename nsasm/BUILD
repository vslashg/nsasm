load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

# To avoid circular dependencies, targets are layered.  No library in one
# layer may depend on entities in a higher layer.  (Test targets may depend on
# anything.)

# Layer 1: Simple data type definitions, plus Error and I/O.

cc_library(
    name = "location",
    hdrs = ["location.h"],
    deps = [
        "@abseil-cpp//absl/strings:str_format",
    ],
)

cc_library(
    name = "error",
    srcs = ["error.cc"],
    hdrs = ["error.h"],
    deps = [
        ":location",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/types:optional",
        "@abseil-cpp//absl/types:variant",
    ],
)

cc_library(
    name = "file",
    srcs = ["file.cc"],
    hdrs = ["file.h"],
    deps = [":error"],
)

cc_library(
    name = "numeric_type",
    hdrs = ["numeric_type.h"],
    deps = [
        ":error",
    ],
)

cc_library(
    name = "address",
    hdrs = ["address.h"],
    deps = [
        ":error",
        ":numeric_type",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "memory",
    hdrs = ["memory.h"],
    srcs = ["memory.cc"],
    deps = [
        ":address",
        ":error",
        "@abseil-cpp//absl/types:span",
    ],
)

cc_library(
    name = "ranges",
    srcs = ["ranges.cc"],
    hdrs = ["ranges.h"],
    deps = [
        ":address",
        "@abseil-cpp//absl/types:optional",
    ],
)

cc_test(
    name = "ranges_test",
    srcs = ["ranges_test.cc"],
    deps = [
        ":ranges",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "mnemonic",
    srcs = ["mnemonic.cc"],
    hdrs = ["mnemonic.h"],
    deps = [
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/types:optional",
    ],
)

cc_test(
    name = "mnemonic_test",
    srcs = ["mnemonic_test.cc"],
    deps = [
        ":mnemonic",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "identifiers",
    hdrs = ["identifiers.h"],
    deps = [
        "@abseil-cpp//absl/strings",
    ],
)

# Other layers

cc_library(
    name = "token",
    srcs = ["token.cc"],
    hdrs = ["token.h"],
    deps = [
        ":error",
        ":mnemonic",
        ":numeric_type",
    ],
)

cc_test(
    name = "token_test",
    srcs = ["token_test.cc"],
    deps = [
        ":token",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "expression",
    srcs = ["expression.cc"],
    hdrs = ["expression.h"],
    deps = [
        ":error",
        ":identifiers",
        ":numeric_type",
        "@abseil-cpp//absl/memory",
        "@abseil-cpp//absl/types:optional",
    ],
)

cc_test(
    name = "expression_test",
    srcs = ["expression_test.cc"],
    deps = [
        ":expression",
        ":parse",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "addressing_mode",
    srcs = ["addressing_mode.cc"],
    hdrs = ["addressing_mode.h"],
    deps = [
        ":error",
        ":expression",
        ":mnemonic",
        "@abseil-cpp//absl/strings:str_format",
    ],
)

cc_test(
    name = "addressing_mode_test",
    srcs = ["addressing_mode_test.cc"],
    deps = [
        ":addressing_mode",
        ":instruction",
        ":opcode_map",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "opcode_map",
    srcs = ["opcode_map.cc"],
    hdrs = ["opcode_map.h"],
    deps = [
        ":addressing_mode",
        ":mnemonic",
        "@abseil-cpp//absl/container:flat_hash_map",
    ],
)

cc_test(
    name = "opcode_map_test",
    srcs = ["opcode_map_test.cc"],
    deps = [
        ":opcode_map",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "execution_state",
    srcs = ["execution_state.cc"],
    hdrs = ["execution_state.h"],
    deps = [
        "@abseil-cpp//absl/container:inlined_vector",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/types:optional",
    ],
)

cc_library(
    name = "calling_convention",
    srcs = ["calling_convention.cc"],
    hdrs = ["calling_convention.h"],
    deps = [
        ":execution_state",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/types:variant",
    ],
)

cc_library(
    name = "instruction",
    srcs = ["instruction.cc"],
    hdrs = ["instruction.h"],
    deps = [
        ":addressing_mode",
        ":calling_convention",
        ":expression",
        ":memory",
        ":mnemonic",
        ":opcode_map",
    ],
)

cc_library(
    name = "decode",
    srcs = ["decode.cc"],
    hdrs = ["decode.h"],
    deps = [
        ":error",
        ":instruction",
        ":opcode_map",
        "@abseil-cpp//absl/memory",
        "@abseil-cpp//absl/types:span",
    ],
)

cc_test(
    name = "decode_test",
    srcs = ["decode_test.cc"],
    deps = [
        ":decode",
        ":opcode_map",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "rom",
    srcs = ["rom.cc"],
    hdrs = ["rom.h"],
    deps = [
        ":error",
        ":memory",
    ],
)

cc_library(
    name = "disassemble",
    srcs = ["disassemble.cc"],
    hdrs = ["disassemble.h"],
    deps = [
        ":decode",
        ":error",
        ":instruction",
        ":rom",
    ],
)

cc_library(
    name = "directive",
    srcs = ["directive.cc"],
    hdrs = ["directive.h"],
    deps = [
        ":calling_convention",
        ":expression",
        ":memory",
        ":mnemonic",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/types:optional",
    ],
)

cc_test(
    name = "directive_test",
    srcs = ["directive_test.cc"],
    deps = [
        ":directive",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "statement",
    srcs = ["statement.cc"],
    hdrs = ["statement.h"],
    deps = [
        ":directive",
        ":instruction",
        ":location",
    ],
)

cc_library(
    name = "parse",
    srcs = ["parse.cc"],
    hdrs = ["parse.h"],
    deps = [
        ":directive",
        ":error",
        ":instruction",
        ":opcode_map",
        ":statement",
        ":token",
    ],
)

cc_test(
    name = "parse_test",
    srcs = ["parse_test.cc"],
    deps = [
        ":parse",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "module",
    srcs = ["module.cc"],
    hdrs = ["module.h"],
    deps = [
        ":file",
        ":parse",
        ":ranges",
        ":statement",
        ":token",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/strings:str_format",
    ],
)

cc_library(
    name = "assembler",
    srcs = ["assembler.cc"],
    hdrs = ["assembler.h"],
    deps = [
        ":error",
        ":module",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
    ],
)
