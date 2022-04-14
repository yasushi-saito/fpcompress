workspace(name = "fpcompress")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

http_archive(
    name = "rules_proto",
    sha256 = "66bfdf8782796239d3875d37e7de19b1d94301e8972b3cbd2446b332429b4df1",
    strip_prefix = "rules_proto-4.0.0",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
        "https://github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
    ],
)
load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
rules_proto_dependencies()
rules_proto_toolchains()

git_repository(
    name = "com_github_cschuet_zstd",
    commit = "0790403a9c2d1f512d012294f304a6bfe97f7487",
    remote = "https://github.com/yasushi-saito/zstd-1.git",
)

load("@com_github_cschuet_zstd//bazel:repositories.bzl", "repositories")

repositories()

http_archive(
    name = "googletest",
    sha256 = "4902bffbb006d9b84cb657afcfc80f2c5cef4a2ac4cf474eaee028c4b38fc371",
    strip_prefix = "googletest-ca4b7c9ff4d8a5c37ac51795b03ffe934958aeff",
    urls = ["https://github.com/google/googletest/archive/ca4b7c9ff4d8a5c37ac51795b03ffe934958aeff.zip"],
)

http_archive(
    name = "com_google_absl",
    urls = ["https://github.com/abseil/abseil-cpp/archive/7c6608d0dbe43cf9bdf7f77787bc6bc89cc42f8b.zip"],
    strip_prefix = "abseil-cpp-7c6608d0dbe43cf9bdf7f77787bc6bc89cc42f8b",
)

#glog

http_archive(
    name = "com_github_gflags_gflags",
    sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
    strip_prefix = "gflags-2.2.2",
    urls = ["https://github.com/gflags/gflags/archive/v2.2.2.tar.gz"],
)

http_archive(
    name = "com_github_google_glog",
    sha256 = "21bc744fb7f2fa701ee8db339ded7dce4f975d0d55837a97be7d46e8382dea5a",
    strip_prefix = "glog-0.5.0",
    urls = ["https://github.com/google/glog/archive/v0.5.0.zip"],
)

http_archive(
    name = "com_github_nlohmann_json",
    build_file_content = """
cc_library(
    name = "json",
    hdrs = ["single_include/nlohmann/json.hpp"],
    strip_include_prefix = "single_include/",
    visibility = ["//visibility:public"],
)
    """,
    sha256 = "5daca6ca216495edf89d167f808d1d03c4a4d929cef7da5e10f135ae1540c7e4",
    strip_prefix = "json-3.10.5",
    urls = ["https://github.com/nlohmann/json/archive/v3.10.5.tar.gz"],
)
