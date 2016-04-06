#include "cc/io/xdfs/xdfs_common.h"

#include "cc/utils/error.h"

using std::string;
using std::to_string;

namespace io {
namespace xdfs {

DirEntry ReadDirEntryAtOffset(File* file, size_t offset) {
  CHECK_ERROR(file->Seek(offset).error());
  DirEntry dir_entry;
  CHECK_ERROR(file->Read(reinterpret_cast<char*>(&dir_entry),
                         kDirEntryMaskSizeBytes).error());
  char name_buffer[kMaxFileNameSizeBytes];
  CHECK_ERROR(file->Read(name_buffer, dir_entry.name_size_bytes).error());
  dir_entry.name = string(name_buffer, dir_entry.name_size_bytes);
  dir_entry.offset_bytes = offset;
  return dir_entry;
}

string ToString(const DirEntry& entry) {
  return string()
      +  "struct DirEntry {\n"
      +  "  left_child_dwords = " + to_string(entry.left_child_dwords) + ";\n"
      +  "  right_child_dwords = " + to_string(entry.right_child_dwords) + ";\n"
      +  "  start_sector = " + to_string(entry.start_sector) + ";\n"
      +  "  size_bytes = " + to_string(entry.size_bytes) + ";\n"
      +  "  attributes = " + to_string(entry.attributes + 0) + ";\n"
      +  "  name_size_bytes = " + to_string(entry.name_size_bytes + 0) + ";\n"
      +  "  offset_bytes = " + to_string(entry.offset_bytes) + ";\n"
      +  "  name = " + entry.name + ";\n"
      +  "}\n";
}

} // namespace xdfs
} // namespace io
