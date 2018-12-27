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
    hdrs=["instruction.h"],
    copts=NSASM_COPTS,
    deps=[
        ":addressing_mode",
        ":argument",
        ":mnemonic",
    ],
)


cc_library(
    name="opcode_map",
    srcs=["opcode_map.cc"],
    hdrs=["opcode_map.h"],
    copts=NSASM_COPTS,
    deps=[
        ":instruction",
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
    name="decode",
    srcs=["decode.cc"],
    hdrs=["decode.h"],
    copts=NSASM_COPTS,
    deps=[
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
        "@absl//absl/types:optional",
    ],
)
