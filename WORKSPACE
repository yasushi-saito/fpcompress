workspace(name = "fpcompress")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

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
