cc_library(
  name = "elf",
  hdrs = ["elf.h"],
  srcs = ["elf.cc"],
  deps = [
    "//cc/exec/xbe:xbe_common",
    "//cc/io:file",
    "//cc/utils:error",
  ],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "make_elf",
  srcs = ["make_elf.cc"],
  deps = [
    "//cc/io:file",
    "//cc/utils:error",
    ":elf",
  ],
)
