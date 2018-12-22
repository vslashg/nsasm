NSASM_COPTS = ["--std=c++14"]

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
    name="addressing_mode",
    srcs=["addressing_mode.cc"],
    hdrs=["addressing_mode.h"],
    copts=NSASM_COPTS,
    deps=[
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
    name="flag_state",
    srcs=["flag_state.cc"],
    hdrs=["flag_state.h"],
    copts=NSASM_COPTS,
    deps=[
        ":mnemonic",
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
