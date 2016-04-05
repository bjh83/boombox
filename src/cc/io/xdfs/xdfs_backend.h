#ifndef IO_XDFS_XDFS_BACKEND_H_
#define IO_XDFS_XDFS_BACKEND_H_

#include <memory>
#include "cc/io/file.h"
#include "cc/io/xdfs/xdfs_common.h"

namespace io {
namespace xdfs {
class XdfsBackend {
 public:
  XdfsBackend(File&& file) : file_(std::move(file)) {}
  
  DirEntry ReadDirEntry(size_t offset_bytes);
  Sector ReadSector(size_t offset_bytes);
 private:
  File file_;
};
} // namespace xdfs
} // namespace io

#endif // IO_XDFS_XDFS_BACKEND_H_
