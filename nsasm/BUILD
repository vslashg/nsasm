NSASM_COPTS = ["--std=c++14"]

package(default_visibility=["//visibility:public"])

cc_library(
    name="mnemonic",
    srcs=["mnemonic.cc"],
    hdrs=["mnemonic.h"],
    copts=NSASM_COPTS,
    deps=[
        "@absl//absl/strings",
        "@absl//absl/types:optional",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_test(
    name="mnemonic_test",
    srcs=["mnemonic_test.cc"],
    copts=NSASM_COPTS,
    deps=[
        ":mnemonic",
        "@absl//absl/strings",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="argument",
    srcs=["argument.cc"],
    hdrs=["argument.h"],
    copts=NSASM_COPTS,
    deps=[
        "@absl//absl/strings",
        "@absl//absl/strings:str_format",
        "@absl//absl/types:optional",
    ],
)


cc_library(
    name="addressing_mode",
    srcs=["addressing_mode.cc"],
    hdrs=["addressing_mode.h"],
    copts=NSASM_COPTS,
    deps=[
        ":argument",
        "@absl//absl/strings:str_format",
    ],
)

cc_test(
    name="addressing_mode_test",
    srcs=["addressing_mode_test.cc"],
    copts=NSASM_COPTS,
    deps=[
        ":addressing_mode",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="instruction",
    srcs=["instruction.cc"],
    hdrs=["instruction.h"],
    copts=NSASM_COPTS,
    deps=[
        ":addressing_mode",
        ":argument",
        ":mnemonic",
    ],
)


cc_library(
    name="flag_state",
    srcs=["flag_state.cc"],
    hdrs=["flag_state.h"],
    copts=NSASM_COPTS,
    deps=[
        ":instruction",
        "@absl//absl/base:core_headers",
    ],
)

cc_test(
    name="flag_state_test",
    srcs=["flag_state_test.cc"],
    copts=NSASM_COPTS,
    deps=[
        ":flag_state",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="opcode_map",
    srcs=["opcode_map.cc"],
    hdrs=["opcode_map.h"],
    copts=NSASM_COPTS,
    deps=[
        ":flag_state",
        ":instruction",
    ],
)

cc_test(
    name="opcode_map_test",
    srcs=["opcode_map_test.cc"],
    copts=NSASM_COPTS,
    deps=[
        ":opcode_map",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="error",
    srcs=["error.cc"],
    hdrs=["error.h"],
    deps=[
        "@absl//absl/strings:str_format",
        "@absl//absl/types:variant",
    ]
)


cc_library(
    name="decode",
    srcs=["decode.cc"],
    hdrs=["decode.h"],
    copts=NSASM_COPTS,
    deps=[
        ":error",
        ":flag_state",
        ":instruction",
        ":opcode_map",
    ],
)


cc_library(
    name="rom",
    srcs=["rom.cc"],
    hdrs=["rom.h"],
    copts=NSASM_COPTS,
    deps=[
        ":error",
    ],
)


cc_library(
    name="disassemble",
    srcs=["disassemble.cc"],
    hdrs=["disassemble.h"],
    copts=NSASM_COPTS,
    deps=[
        ":decode",
        ":error",
        ":flag_state",
        ":instruction",
        ":rom",
    ],
)