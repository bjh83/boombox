#include "cc/io/xdfs/xdfs_dir.h"

#include <string>

using std::string;
using std::vector;
using utils::ErrorOr;

namespace io {
namespace xdfs {
namespace {
vector<XdfsDirEntry> ReadEntriesImpl(XdfsBackend* backend, size_t offset_bytes) {
  vector<XdfsDirEntry> results;
  vector<size_t> offsets_to_scan = { offset_bytes };
  while (!offsets_to_scan.empty()) {
    const DirEntry entry = backend->ReadDirEntry(offsets_to_scan.back());
    offsets_to_scan.pop_back();
    results.push_back({entry.name, entry.attributes});
    if (entry.left_child_dwords > 0) {
      offsets_to_scan.push_back(
          entry.left_child_dwords * kDWordsBytes + offset_bytes);
    }
    if (entry.right_child_dwords > 0) {
      offsets_to_scan.push_back(
          entry.right_child_dwords * kDWordsBytes + offset_bytes);
    }
  }
  return results;
}
} // namespace1

ErrorOr<vector<XdfsDirEntry>> XdfsDir::ReadEntries() {
  if (!entries_read_) {
    cached_entries_ = ReadEntriesImpl(xdfs_backend_,
                                      SectorToOffset(attributes_.start_sector));
    entries_read_ = true;
  }
  vector<XdfsDirEntry> copy = cached_entries_;
  return ErrorOr<vector<XdfsDirEntry>>(std::move(copy));
}

} // namespace xdfs
} // namespace io
