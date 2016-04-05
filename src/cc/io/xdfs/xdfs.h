#ifndef IO_XDFS_XDFS_H_
#define IO_XDFS_XDFS_H_

#include <memory>
#include <string>
#include "cc/io/file.h"
#include "cc/io/xdfs/xdfs_backend.h"
#include "cc/io/xdfs/xdfs_common.h"
#include "cc/io/xdfs/xdfs_dir.h"
#include "cc/io/xdfs/xdfs_file.h"
#include "cc/utils/error.h"

namespace io {
namespace xdfs {
class Xdfs {
 public:
  static utils::ErrorOr<Xdfs> CreateXdfs(File&& file);

  utils::ErrorOr<XdfsFile> OpenFile(const std::string& path);
  utils::ErrorOr<XdfsDir> OpenDir(const std::string& path);

 private:
  XdfsBackend xdfs_backend_;
  const DirEntry root_entry_;

  Xdfs(File&& file, DirEntry root_entry)
      : xdfs_backend_(std::move(file)), root_entry_(root_entry) {}

  utils::ErrorOr<bool> DirEntryFromPath(DirEntry* entry,
                                        const std::string& path);
  utils::ErrorOr<bool> LookupDirEntryInTable(DirEntry* entry,
                                             const DirEntry& root,
                                             const std::string& dir_name);
};
} // namespace xdfs
} // namespace io

#endif // IO_XDFS_XDFS_H_
