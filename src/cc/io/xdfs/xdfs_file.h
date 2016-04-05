#ifndef IO_XDFS_XDFS_FILE_H_
#define IO_XDFS_XDFS_FILE_H_

#include "cc/io/xdfs/xdfs_backend.h"
#include "cc/io/xdfs/xdfs_common.h"
#include "cc/utils/error.h"

namespace io {
namespace xdfs {
class XdfsFile {
 public:
  utils::ErrorOr<ssize_t> Read(char* buffer, size_t max_to_read);
  utils::ErrorOr<ssize_t> Seek(size_t offset);

 private:
  DirEntry attributes_;
  XdfsBackend* xdfs_backend_;
  // Current offset within file.
  size_t current_offset_ = 0;
  // Offset of sector start relative to start of file.
  ssize_t sector_offset_ = -1;
  // The most recent sector read.
  Sector current_sector_;
  
  XdfsFile(DirEntry attributes, XdfsBackend* xdfs_backend)
      : attributes_(attributes), xdfs_backend_(xdfs_backend) {}

  friend class Xdfs;
};
} // namespace xdfs
} // namespace io

#endif // IO_XDFS_XDFS_FILE_H_
