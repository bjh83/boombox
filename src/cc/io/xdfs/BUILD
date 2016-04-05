cc_library(
  name = "xdfs",
  hdrs = ["xdfs.h"],
  srcs = ["xdfs.cc"],
  deps = [
    "//cc/io:file",
    "//cc/utils:error",
    ":xdfs_backend",
    ":xdfs_common",
    ":xdfs_dir",
    ":xdfs_file",
  ],
)

cc_library(
  name = "xdfs_dir",
  hdrs = ["xdfs_dir.h"],
  srcs = ["xdfs_dir.cc"],
  deps = [
    "//cc/utils:error",
    ":xdfs_backend",
    ":xdfs_common",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "xdfs_file",
  hdrs = ["xdfs_file.h"],
  srcs = ["xdfs_file.cc"],
  deps = [
    "//cc/utils:error",
    ":xdfs_backend",
    ":xdfs_common",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "xdfs_backend",
  hdrs = ["xdfs_backend.h"],
  srcs = ["xdfs_backend.cc"],
  deps = [
    "//cc/io:file",
    ":xdfs_common",
  ],
)

cc_library(
  name = "xdfs_common",
  hdrs = ["xdfs_common.h"],
  srcs = ["xdfs_common.cc"],
  deps = [
    "//cc/io:file",
    "//cc/utils:error",
  ],
)

cc_binary(
  name = "print_files",
  srcs = ["print_files.h"],
  deps = [
    "//cc/io:file",
    "//cc/utils:error",
    ":xdfs",
    ":xdfs_dir",
  ],
)
