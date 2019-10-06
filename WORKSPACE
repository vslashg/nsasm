workspace(name = "com_verylowsodium_nsasm")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# abseil
http_archive(
    name = "absl",
    strip_prefix = "abseil-cpp-master",
    urls = ["https://github.com/abseil/abseil-cpp/archive/master.zip"],
)

# Google Test
http_archive(
    name = "gtest",
    strip_prefix = "googletest-master",
    urls = ["https://github.com/google/googletest/archive/master.zip"],
)

# Skylark rules
http_archive(
    name = "rules_cc",
    sha256 = "36fa66d4d49debd71d05fba55c1353b522e8caef4a20f8080a3d17cdda001d89",
    strip_prefix = "rules_cc-0d5f3f2768c6ca2faca0079a997a97ce22997a0c",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_cc/archive/0d5f3f2768c6ca2faca0079a997a97ce22997a0c.zip",
        "https://github.com/bazelbuild/rules_cc/archive/0d5f3f2768c6ca2faca0079a997a97ce22997a0c.zip",
    ],
)
