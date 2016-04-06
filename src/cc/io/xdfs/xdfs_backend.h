#ifndef IO_XDFS_XDFS_BACKEND_H_
#define IO_XDFS_XDFS_BACKEND_H_

#include <memory>
#include "cc/io/file.h"
#include "cc/io/xdfs/xdfs_common.h"
#include "cc/utils/error.h"

namespace io {
namespace xdfs {
class XdfsBackend {
 public:
  XdfsBackend(File&& file) : file_(std::move(file)) {}
  XdfsBackend(XdfsBackend&& xdfs_backend)
      : file_(std::move(xdfs_backend.file_)) {}
  
  utils::ErrorOr<DirEntry> ReadDirEntry(size_t offset_bytes);
  utils::ErrorOr<Sector> ReadSector(size_t offset_bytes);
 private:
  File file_;

  XdfsBackend(const XdfsBackend&) = delete;
  XdfsBackend& operator=(const XdfsBackend&) = delete;
};
} // namespace xdfs
} // namespace io

#endif // IO_XDFS_XDFS_BACKEND_H_
