#include "cc/io/xdfs/xdfs.h"

#include <algorithm>
#include <cctype>
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
static const size_t kVolumeDescriptorPart2OffsetFromDescriptorStartBytes = 2028;
static const size_t kVolumeDescriptorPart2OffsetAbsoluteBytes =
    kVolumeDescriptorOffsetBytes
    + kVolumeDescriptorPart2OffsetFromDescriptorStartBytes;

static const string kMicrosoftXboxMedia = "MICROSOFT*XBOX*MEDIA";
static const size_t kMicrosoftXboxMediaSize = 20;

struct VolumeDescriptor {
  char microsoft_xbox_media[20];
  uint32_t root_directory_sector;
  uint32_t root_directory_size_bytes;
  char file_creation_time[8];
};

struct VolumeDescriptorPart2 {
  char microsoft_xbox_media[20];
};

ErrorOr<VolumeDescriptor> ReadVolumeDescriptorAndVerify(File* file) {
  PASS_ERROR(file->Seek(kVolumeDescriptorOffsetBytes).error());
  VolumeDescriptor descriptor;
  PASS_ERROR(
      file->Read(reinterpret_cast<char*>(&descriptor), sizeof(descriptor))
      .error());
  RETURN_ERROR_IF_NOT(
      kMicrosoftXboxMedia == string(descriptor.microsoft_xbox_media,
                                    kMicrosoftXboxMediaSize),
      "Volume descriptor does not exist or is not formatted properly.");

  CHECK_ERROR(file->Seek(kVolumeDescriptorPart2OffsetAbsoluteBytes).error());
  VolumeDescriptorPart2 descriptor_part_2;
  CHECK_ERROR(file->Read(reinterpret_cast<char*>(&descriptor_part_2),
                         sizeof(VolumeDescriptorPart2))
              .error());
  RETURN_ERROR_IF_NOT(
      kMicrosoftXboxMedia == string(descriptor_part_2.microsoft_xbox_media,
                                    kMicrosoftXboxMediaSize),
      "Volume descriptor does not exist or is not formatted properly.");

  return ErrorOr<VolumeDescriptor>(std::move(descriptor));
}

vector<string> SplitPath(const string& path) {
  size_t end_pos = path.size();
  if (path.back() == '/') {
    end_pos--;
  }
  vector<string> split_path;
  std::stringstream path_stream(path.substr(1, end_pos));
  string component;
  while (std::getline(path_stream, component, '/')) {
    split_path.push_back(component);
  }
  return split_path;
}

bool CaseInsensitiveCompare(char left, char right) {
  return std::toupper(left) < std::toupper(right);
}
} // namespace

ErrorOr<Xdfs> Xdfs::CreateXdfs(File&& file) {
  ErrorOr<VolumeDescriptor> error_or_descriptor =
      ReadVolumeDescriptorAndVerify(&file);
  PASS_ERROR(error_or_descriptor.error());
  DirEntry root_entry;
  root_entry.left_child_dwords = 0;
  root_entry.right_child_dwords = 0;
  root_entry.start_sector = error_or_descriptor.get().root_directory_sector;
  root_entry.size_bytes = error_or_descriptor.get().root_directory_size_bytes;
  root_entry.attributes = kAttributeIsDirectoryMask;
  root_entry.name_size_bytes = 1;
  root_entry.offset_bytes = kVolumeDescriptorOffsetBytes;
  root_entry.name = "/";
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
  RETURN_ERROR_IF(path[0] != '/', "Must specify full path.");

  DirEntry current_entry = root_entry_;
  for (const string& path_component : SplitPath(path)) {
    if (!IsDir(current_entry.attributes)) {
      // We still have more of the path to resolve, but the current entry
      // is not a directory.
      return ErrorOr<bool>(false);
    }
    ErrorOr<DirEntry> error_or_dir_entry =
        xdfs_backend_.ReadDirEntry(SectorToOffset(current_entry.start_sector));
    PASS_ERROR(error_or_dir_entry.error());
    ErrorOr<bool> error_or_is_found =
        LookupDirEntryInTable(&current_entry,
                              error_or_dir_entry.get(),
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
  size_t dir_offset = root.offset_bytes;
  while (current_entry.name != dir_name) {
    if (std::lexicographical_compare(dir_name.begin(),
                                     dir_name.end(),
                                     current_entry.name.begin(),
                                     current_entry.name.end(),
                                     CaseInsensitiveCompare)) {
      if (current_entry.left_child_dwords == 0) {
        return ErrorOr<bool>(false);
      }
      ErrorOr<DirEntry> error_or_dir_entry = xdfs_backend_.ReadDirEntry(
          dir_offset + current_entry.left_child_dwords * kDWordsBytes);
      PASS_ERROR(error_or_dir_entry.error());
      current_entry = error_or_dir_entry.move();
    } else {
      if (current_entry.right_child_dwords == 0) {
        return ErrorOr<bool>(false);
      }
      ErrorOr<DirEntry> error_or_dir_entry = xdfs_backend_.ReadDirEntry(
          dir_offset + current_entry.right_child_dwords * kDWordsBytes);
      PASS_ERROR(error_or_dir_entry.error());
      current_entry = error_or_dir_entry.move();
    }
  }
  *entry = current_entry;
  return ErrorOr<bool>(true);
}
} // namespace xdfs
} // namespace io
