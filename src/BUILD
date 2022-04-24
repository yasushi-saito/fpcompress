load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_proto//proto:defs.bzl", "proto_library")

cc_library(
    name = "fpcompress",
    srcs = [
        "zstd_stream.h",
        "output_stream.h",
        "input_stream.h",
        "columnar_stream.h",
        "dfcm_predictor.h",
        "last_value_predictor.h",
        "rotating_encoder.h",
        "lorenzo_encoder.h",
        "front.h",
        "stride_predictor.h",
        "zero_predictor.h",
        "id_encoder.h",
        "bit_cast.h",
        "zstd_stream.cc",
        "parse.h",
        "file_stream.h",
        "file_stream.cc",
        "combo_stream.h",
        "combo_stream.cc",
        "memory_stream.h",
        "const.h",
        "memory_stream.cc",
    ],
    deps = [
        ":fpcompress_cc_proto",
        "@com_google_absl//absl/types:span",
        "@com_github_facebook_zstd//:libzstd",
        "@com_github_google_glog//:glog",
    ],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "main",
    srcs = ["fpcompress.cc"],
    deps = [
        ":fpcompress",
        "@com_google_absl//absl/flags:flag",
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

cc_test(
    name = "stream_test",
    srcs = ["stream_test.cc"],
    deps = [
        ":fpcompress",
        "@com_google_absl//absl/strings:str_format",
        "@googletest//:gtest_main",
    ],
)

proto_library(
    name = "fpcompress_proto",
    srcs = ["fpcompress.proto"],
)

cc_proto_library(
    name = "fpcompress_cc_proto",
    deps = [":fpcompress_proto"],
)
