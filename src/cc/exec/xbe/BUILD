cc_library(
  name = "xbe_common",
  hdrs = ["xbe_common.h"],
  srcs = ["xbe_common.cc"],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "print_xbe",
  srcs = ["print_xbe.cc"],
  deps = [
    "//cc/io:file",
    "//cc/utils:error",
    ":xbe_common",
  ],
)
