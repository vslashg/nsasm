load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "print_opcode_map",
    srcs = ["print_opcode_map.cc"],
    deps = [
        "//nsasm:instruction",
        "//nsasm:opcode_map",
        "@absl//absl/strings:str_format",
    ],
)

cc_binary(
    name = "quick_disassemble",
    srcs = ["quick_disassemble.cc"],
    deps = [
        "//nsasm:decode",
        "//nsasm:disassemble",
        "//nsasm:rom",
        "@absl//absl/strings",
        "@absl//absl/strings:str_format",
    ],
)

cc_binary(
    name = "quick_dump",
    srcs = ["quick_dump.cc"],
    deps = [
        "//nsasm:rom",
        "@absl//absl/strings",
        "@absl//absl/strings:str_format",
    ],
)

cc_binary(
    name = "quick_assemble",
    srcs = ["quick_assemble.cc"],
    deps = [
        "//nsasm:assembler",
        "//nsasm:rom",
        "@absl//absl/strings:str_format",
    ],
)

cc_binary(
    name = "interactive_parse",
    srcs = ["interactive_parse.cc"],
    deps = [
        "//nsasm:error",
        "//nsasm:parse",
        "//nsasm:token",
    ],
)

cc_binary(
    name = "disassemble_more",
    srcs = ["disassemble_more.cc"],
    deps = [
        "//nsasm:assembler",
        "//nsasm:disassemble",
        "//nsasm:rom",
        "@absl//absl/strings:str_format",
    ],
)

cc_binary(
    name = "dumb_disassemble",
    srcs = ["dumb_disassemble.cc"],
    deps = [
        "//nsasm:disassemble",
        "//nsasm:rom",
        "@absl//absl/strings:str_format",
    ],
)
