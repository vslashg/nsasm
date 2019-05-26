package(default_visibility=["//visibility:public"])

cc_library(
    name="mnemonic",
    srcs=["mnemonic.cc"],
    hdrs=["mnemonic.h"],
    deps=[
        "@absl//absl/strings",
        "@absl//absl/types:optional",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_test(
    name="mnemonic_test",
    srcs=["mnemonic_test.cc"],
    deps=[
        ":mnemonic",
        "@absl//absl/strings",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="numeric_type",
    srcs=[],
    hdrs=["numeric_type.h"],
    deps=[
        ":error",
    ],
)


cc_library(
    name="expression",
    srcs=["expression.cc"],
    hdrs=["expression.h"],
    deps=[
        ":error",
        ":numeric_type",
        "@absl//absl/types:optional",
    ],
)

cc_test(
    name="expression_test",
    srcs=["expression_test.cc"],
    deps=[
        ":expression",
        ":parse",
        "@gtest//:gtest",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="addressing_mode",
    srcs=["addressing_mode.cc"],
    hdrs=["addressing_mode.h"],
    deps=[
        ":error",
        ":expression",
        ":mnemonic",
        "@absl//absl/strings:str_format",
    ],
)

cc_test(
    name="addressing_mode_test",
    srcs=["addressing_mode_test.cc"],
    deps=[
        ":addressing_mode",
        ":flag_state",
        ":instruction",
        ":opcode_map",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="opcode_map",
    srcs=["opcode_map.cc"],
    hdrs=["opcode_map.h"],
    deps=[
        ":addressing_mode",
        ":mnemonic",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_test(
    name="opcode_map_test",
    srcs=["opcode_map_test.cc"],
    deps=[
        ":opcode_map",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="flag_state",
    srcs=["flag_state.cc"],
    hdrs=["flag_state.h"],
    deps=[
        "@absl//absl/base:core_headers",
        "@absl//absl/strings",
        "@absl//absl/types:optional",
    ],
)

cc_test(
    name="flag_state_test",
    srcs=["flag_state_test.cc"],
    deps=[
        ":flag_state",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="output_sink",
    hdrs=["output_sink.h"],
    deps=[
        ":error",
        "@absl//absl/types:span",
    ]
)


cc_library(
    name="instruction",
    srcs=["instruction.cc"],
    hdrs=["instruction.h"],
    deps=[
        ":addressing_mode",
        ":expression",
        ":flag_state",
        ":mnemonic",
        ":opcode_map",
        ":output_sink",
    ],
)


cc_library(
    name="error",
    srcs=["error.cc"],
    hdrs=["error.h"],
    deps=[
        "@absl//absl/strings:str_format",
        "@absl//absl/types:optional",
        "@absl//absl/types:variant",
    ]
)


cc_library(
    name="decode",
    srcs=["decode.cc"],
    hdrs=["decode.h"],
    deps=[
        "@absl//absl/memory",
        "@absl//absl/types:span",
        ":error",
        ":flag_state",
        ":instruction",
        ":opcode_map",
    ],
)

cc_test(
    name="decode_test",
    srcs=["decode_test.cc"],
    deps=[
        ":decode",
        ":opcode_map",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="rom",
    srcs=["rom.cc"],
    hdrs=["rom.h"],
    deps=[
        ":error",
        ":output_sink",
    ],
)


cc_library(
    name="disassemble",
    srcs=["disassemble.cc"],
    hdrs=["disassemble.h"],
    deps=[
        ":decode",
        ":error",
        ":flag_state",
        ":instruction",
        ":rom",
    ],
)


cc_library(
    name="directive",
    srcs=["directive.cc"],
    hdrs=["directive.h"],
    deps=[
        ":expression",
        ":flag_state",
        ":output_sink",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/strings",
        "@absl//absl/types:optional",
    ],
)

cc_test(
    name="directive_test",
    srcs=["directive_test.cc"],
    deps=[
        ":directive",
        "@absl//absl/strings",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="token",
    srcs=["token.cc"],
    hdrs=["token.h"],
    deps=[
        ":directive",
        ":error",
        ":mnemonic",
        ":numeric_type",
    ],
)

cc_test(
    name="token_test",
    srcs=["token_test.cc"],
    deps=[
        ":token",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="statement",
    srcs=["statement.cc"],
    hdrs=["statement.h"],
    deps=[
        ":directive",
        ":instruction",
    ],
)


cc_library(
    name="parse",
    srcs=["parse.cc"],
    hdrs=["parse.h"],
    deps=[
        ":directive",
        ":error",
        ":instruction",
        ":opcode_map",
        ":statement",
        ":token",
    ],
)

cc_test(
    name="parse_test",
    srcs=["parse_test.cc"],
    deps=[
        ":parse",
        "@gtest//:gtest_main",
    ],
)


cc_library(
    name="module",
    srcs=["module.cc"],
    hdrs=["module.h"],
    deps=[
        ":parse",
        ":statement",
        ":token",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/strings:str_format",
    ],
)


cc_library(
    name="assembler",
    srcs=["assembler.cc"],
    hdrs=["assembler.h"],
    deps=[
        ":error",
        ":module",
        "@absl//absl/container:flat_hash_map",
    ],
)
