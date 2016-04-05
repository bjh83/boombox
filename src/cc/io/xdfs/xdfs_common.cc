#include "cc/io/xdfs/xdfs_common.h"

#include "cc/utils/error.h"

using std::string;

namespace io {
namespace xdfs {

DirEntry ReadDirEntryAtOffset(File* file, size_t offset) {
  CHECK_ERROR(file->Seek(offset));
  DirEntry dir_entry;
  CHECK_ERROR(file->Read(reinterpret_cast<char*>(&dir_entry),
                         kDirEntryMaskSizeBytes));
  char name_buffer[kMaxFileNameSizeBytes];
  CHECK_ERROR(file->Read(name_buffer, dir_entry.name_size_bytes));
  dir_entry.name = string(name_buffer, dir_entry.name_size_bytes);
  dir_entry.offset_bytes = offset;
  return dir_entry;
}

} // namespace xdfs
} // namespace io
