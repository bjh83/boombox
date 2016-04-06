#include "cc/io/xdfs/xdfs_backend.h"

using utils::ErrorOr;

namespace io {
namespace xdfs {

ErrorOr<DirEntry> XdfsBackend::ReadDirEntry(size_t offset_bytes) {
  return ReadDirEntryAtOffset(&file_, offset_bytes);
}

ErrorOr<Sector> XdfsBackend::ReadSector(size_t offset_bytes) {
  PASS_ERROR(file_.Seek(offset_bytes).error());
  Sector sector;
  PASS_ERROR(file_.Read(reinterpret_cast<char*>(&sector),
                        sizeof(Sector)).error());
  return std::move(sector);
}

} // namespace xdfs
} // namespace io
