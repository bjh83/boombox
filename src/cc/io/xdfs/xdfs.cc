#include "cc/io/xdfs/xdfs.h"

#include <cstdint>
#include <sstream>
#include <vector>

using std::string;
using std::vector;
using utils::ErrorOr;

namespace io {
namespace xdfs {
namespace {
static const size_t kVolumeDescriptorOffsetBytes = 65536;
static const string kMicrosoftXboxMedia = "MICROSOFT*XBOX*MEDIA";
static const size_t kMicrosoftXboxMediaSize = 20;
static const size_t kVolumeDescriptorPaddingBytes = 1992;

struct VolumeDescriptor {
  char microsoft_xbox_media[20];
  uint32_t root_directory_sector;
  uint32_t root_directory_size_bytes;
  char file_creation_time[8];
};

static const size_t kVolumeDescriptorPart2OffsetBytes =
    kVolumeDescriptorOffsetBytes + sizeof(VolumeDescriptor) 
    + kVolumeDescriptorOffsetBytes;

struct VolumeDescriptorPart2 {
  char microsoft_xbox_media[20];
};

ErrorOr<VolumeDescriptor> ReadVolumeDescriptorAndVerify(File* file) {
  CHECK_ERROR(file->Seek(kVolumeDescriptorOffsetBytes));
  VolumeDescriptor descriptor;
  CHECK_ERROR(
      file->Read(reinterpret_cast<char*>(&descriptor), sizeof(descriptor)));
  RETURN_ERROR_IF_NOT(
      kMicrosoftXboxMedia == string(descriptor.microsoft_xbox_media,
                                    kMicrosoftXboxMediaSize),
      "Volume descriptor does not exist or is not formatted properly.");

  CHECK_ERROR(file->Seek(kVolumeDescriptorPart2OffsetBytes));
  VolumeDescriptorPart2 descriptor_part_2;
  CHECK_ERROR(file->Read(reinterpret_cast<char*>(&descriptor_part_2),
                         sizeof(VolumeDescriptorPart2)));
  RETURN_ERROR_IF_NOT(
      kMicrosoftXboxMedia == string(descriptor_part_2.microsoft_xbox_media,
                                    kMicrosoftXboxMediaSize),
      "Volume descriptor does not exist or is not formatted properly.");

  return ErrorOr<VolumeDescriptor>(std::move(descriptor));
}

vector<string> SplitPath(const string& path) {
  vector<string> split_path;
  std::stringstream path_stream(path);
  string component;
  while (std::getline(path_stream, component, '/')) {
    split_path.push_back(component);
  }
  return split_path;
}
} // namespace

ErrorOr<Xdfs> Xdfs::CreateXdfs(File&& file) {
  ErrorOr<VolumeDescriptor> error_or_descriptor =
      ReadVolumeDescriptorAndVerify(&file);
  PASS_ERROR(error_or_descriptor.error());
  size_t root_dir_offset =
      SectorToOffset(error_or_descriptor.get().root_directory_sector);
  DirEntry root_entry = ReadDirEntryAtOffset(&file, root_dir_offset);
  return ErrorOr<Xdfs>(Xdfs(std::move(file), root_entry));
}

ErrorOr<XdfsFile> Xdfs::OpenFile(const string& path) {
  DirEntry entry;
  ErrorOr<bool> error_or_is_found = DirEntryFromPath(&entry, path);
  PASS_ERROR(error_or_is_found.error());
  RETURN_ERROR_IF_NOT(error_or_is_found.get(), "Could not find given file.");
  RETURN_ERROR_IF(IsDir(entry.attributes), "Requested path is dir not file.");
  return ErrorOr<XdfsFile>(XdfsFile(entry, &xdfs_backend_));
}

ErrorOr<XdfsDir> Xdfs::OpenDir(const string& path) {
  DirEntry entry;
  ErrorOr<bool> error_or_is_found = DirEntryFromPath(&entry, path);
  PASS_ERROR(error_or_is_found.error());
  RETURN_ERROR_IF_NOT(error_or_is_found.get(), "Could not find given file.");
  RETURN_ERROR_IF_NOT(IsDir(entry.attributes),
                      "Requested path is file not dir.");
  return ErrorOr<XdfsDir>(XdfsDir(entry, &xdfs_backend_));
}

ErrorOr<bool> Xdfs::DirEntryFromPath(DirEntry* entry, const string& path) {
  DirEntry current_entry = root_entry_;
  for (const string& path_component : SplitPath(path)) {
    if (!IsDir(current_entry.attributes)) {
      // We still have more of the path to resolve, but the current entry
      // is not a directory.
      return ErrorOr<bool>(false);
    }
    ErrorOr<bool> error_or_is_found = LookupDirEntryInTable(&current_entry,
                                                            current_entry,
                                                            path_component);
    PASS_ERROR(error_or_is_found.error());
    if (!error_or_is_found.get()) {
      return ErrorOr<bool>(false);
    }
  }
  *entry = current_entry;
  return ErrorOr<bool>(true);
}

ErrorOr<bool> Xdfs::LookupDirEntryInTable(DirEntry* entry,
                                          const DirEntry& root,
                                          const string& dir_name) {
  DirEntry current_entry = root;
  while (current_entry.name != dir_name) {
    if (dir_name < current_entry.name) {
      if (current_entry.left_child_dwords == 0) {
        return ErrorOr<bool>(false);
      }
      current_entry = xdfs_backend_.ReadDirEntry(
          current_entry.offset_bytes
          + current_entry.left_child_dwords * kDWordsBytes);
    } else {
      if (current_entry.right_child_dwords == 0) {
        return ErrorOr<bool>(false);
      }
      current_entry = xdfs_backend_.ReadDirEntry(
          current_entry.offset_bytes
          + current_entry.right_child_dwords * kDWordsBytes);
    }
  }
  *entry = current_entry;
  return ErrorOr<bool>(true);
}
} // namespace xdfs
} // namespace io
