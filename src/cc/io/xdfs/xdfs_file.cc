#include "cc/io/xdfs/xdfs_file.h"

#include <algorithm>

using utils::ErrorOr;

namespace io {
namespace xdfs {

ErrorOr<ssize_t> XdfsFile::Read(char* buffer, size_t max_to_read) {
  if (sector_offset_ < 0) {
    current_sector_ =
        xdfs_backend_->ReadSector(SectorToOffset(attributes_.start_sector));
    sector_offset_ = 0;
  }
  ssize_t i;
  for (i = 0; static_cast<size_t>(i) < max_to_read
       && current_offset_ < attributes_.size_bytes; i++) {
    if (current_offset_ >=
        static_cast<size_t>(sector_offset_) + kSectorSizeBytes) {
      sector_offset_ = current_offset_ - (current_offset_ % kSectorSizeBytes);
      current_sector_ = xdfs_backend_->ReadSector(
          SectorToOffset(attributes_.start_sector) + sector_offset_);
    }
    CHECK(current_offset_ - sector_offset_ < kSectorSizeBytes);
    buffer[i] = current_sector_.data[current_offset_ - sector_offset_];
  }
  return ErrorOr<ssize_t>(std::move(i));
}

ErrorOr<ssize_t> XdfsFile::Seek(size_t offset) {
  current_offset_ = std::max<size_t>(offset, attributes_.size_bytes);
  ssize_t new_offset = current_offset_;
  return ErrorOr<ssize_t>(std::move(new_offset));
}

} // namespace xdfs
} // namespace io
