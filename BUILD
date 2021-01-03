load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "fpcompress",
    srcs = [
        "columnar.h",
        "dfcm_predictor.h",
        "last_value_predictor.h",
        "stride_predictor.h",
        "zero_predictor.h",
        "main.cc",
        "type.h",
        "zstd.cc",
        "parse.h",
    ],
    deps = [
        "@com_github_facebook_zstd//:libzstd",
    ],
)

cc_test(
    name = "fpcompress_test",
    srcs = ["fpcompress_test.cc"],
    deps = [
        ":fpcompress",
        "@googletest//:gtest_main",
    ],
)
