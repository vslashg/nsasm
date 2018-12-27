NSASM_COPTS = ["--std=c++14"]

cc_binary(
    name="print_opcode_map",
    srcs=["print_opcode_map.cc"],
    copts=NSASM_COPTS,
    deps=[
        "//nsasm:opcode_map",
        "@absl//absl/strings:str_format",
    ],
)

cc_binary(
    name="quick_disassemble",
    srcs=["quick_disassemble.cc"],
    copts=NSASM_COPTS,
    deps=[
        "@absl//absl/strings",
        "@absl//absl/strings:str_format",
        "//nsasm:decode",
        "//nsasm:disassemble",
        "//nsasm:rom",
    ],
)
