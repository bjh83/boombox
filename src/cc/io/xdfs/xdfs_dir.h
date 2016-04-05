#ifndef IO_XDFS_XDFS_DIR_H_
#define IO_XDFS_XDFS_DIR_H_

#include <string>
#include <vector>
#include "cc/io/xdfs/xdfs_backend.h"
#include "cc/io/xdfs/xdfs_common.h"
#include "cc/utils/error.h"

namespace io {
namespace xdfs {

struct XdfsDirEntry {
  std::string file_name;
  uint8_t attributes;
};

class XdfsDir {
 public:
  utils::ErrorOr<std::vector<XdfsDirEntry>> ReadEntries();

 private:
  DirEntry attributes_;
  XdfsBackend* xdfs_backend_;
  bool entries_read_ = false;
  std::vector<XdfsDirEntry> cached_entries_;

  XdfsDir(DirEntry attributes, XdfsBackend* xdfs_backend)
      : attributes_(attributes), xdfs_backend_(xdfs_backend) {}

  friend class Xdfs;
};
} // namespace xdfs
} // namespace io

#endif // IO_XDFS_XDFS_DIR_H_
