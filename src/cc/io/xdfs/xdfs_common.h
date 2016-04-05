#ifndef IO_XDFS_XDFS_COMMON_H_
#define IO_XDFS_XDFS_COMMON_H_

#include <cstdint>
#include <string>

#include "cc/io/file.h"

namespace io {
namespace xdfs {

static const uint32_t kDirEntryMaskSizeBytes = 14;
static const uint32_t kDWordsBytes = 4;
static const uint32_t kSectorSizeBytes = 2048;
static const uint32_t kMaxFileNameSizeBytes = 256;
static const uint8_t kAttributeIsReadOnlyMask   = 0b00000001;
static const uint8_t kAttributeIsHiddenMask     = 0b00000010;
static const uint8_t kAttributeIsSystemFileMask = 0b00000100;
static const uint8_t kAttributeIsDirectoryMask  = 0b00010000;
static const uint8_t kAttributeIsArchiveMask    = 0b00100000;
static const uint8_t kAttributeIsNormalMask     = 0b10000000;

struct DirEntry {
  uint16_t left_child_dwords;
  uint16_t right_child_dwords;
  uint32_t start_sector;
  uint32_t size_bytes;
  uint8_t attributes;
  uint8_t name_size_bytes;
  size_t offset_bytes;
  std::string name;
};

DirEntry ReadDirEntryAtOffset(File* file, size_t offset);

std::string ToString(const DirEntry& entry);

struct Sector {
  uint8_t data[kSectorSizeBytes];
};

inline size_t SectorToOffset(uint32_t sector_number) {
  return sector_number * kSectorSizeBytes;
}

inline bool IsDir(uint8_t attributes) {
  return (attributes & kAttributeIsDirectoryMask) > 0;
}

} // namespace xdfs
} // namespace io

#endif // IO_XDFS_XDFS_COMMON_H_
