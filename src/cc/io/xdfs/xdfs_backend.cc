#include "cc/io/xdfs/xdfs_backend.h"

namespace io {
namespace xdfs {

DirEntry XdfsBackend::ReadDirEntry(size_t offset_bytes) {
  return ReadDirEntryAtOffset(&file_, offset_bytes);
}

Sector XdfsBackend::ReadSector(size_t offset_bytes) {
  CHECK_ERROR(file_.Seek(offset_bytes));
  Sector sector;
  CHECK_ERROR(file_.Read(reinterpret_cast<char*>(&sector), sizeof(Sector)));
  return sector;
}

} // namespace xdfs
} // namespace io
