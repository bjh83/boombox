#include <iostream>
#include <string>
#include <vector>

#include "cc/io/file.h"
#include "cc/io/xdfs/xdfs.h"
#include "cc/io/xdfs/xdfs_dir.h"
#include "cc/utils/error.h"

using std::string;
using std::vector;
using io::File;
using io::xdfs::IsDir;
using io::xdfs::Xdfs;
using io::xdfs::XdfsDir;
using io::xdfs::XdfsDirEntry;
using utils::ErrorOr;

vector<string> FindAllFilePaths(Xdfs* xdfs) {
  vector<string> all_file_paths;
  vector<string> dirs_to_search;
  dirs_to_search.push_back("/");
  while (!dirs_to_search.empty()) {
    const string current_dir_path = dirs_to_search.back();
    dirs_to_search.pop_back();

    ErrorOr<XdfsDir> error_or_dir = xdfs->OpenDir(current_dir_path);
    CHECK_ERROR(error_or_dir.error());
    XdfsDir dir = error_or_dir.move();

    ErrorOr<vector<XdfsDirEntry>> error_or_dir_entries = dir.ReadEntries();
    CHECK_ERROR(error_or_dir_entries.error());
    for (const XdfsDirEntry& entry : error_or_dir_entries.get()) {
      string new_path = current_dir_path + entry.file_name;
      if (IsDir(entry.attributes)) {
        new_path += "/";
        dirs_to_search.push_back(new_path);
      }
      all_file_paths.push_back(new_path);
    }
  }
  return all_file_paths;
}

int main(int argc, char* argv[]) {
  CHECK_INFO(argc == 2, "Path to ISO must be provided.");
  ErrorOr<File> error_or_file = File::Open(argv[1], File::RD_ONLY);
  CHECK_ERROR(error_or_file.error());
  ErrorOr<Xdfs> error_or_xdfs = Xdfs::CreateXdfs(error_or_file.move());
  CHECK_ERROR(error_or_xdfs.error());
  Xdfs xdfs = error_or_xdfs.move();
  vector<string> all_file_paths = FindAllFilePaths(&xdfs);
  for (const string& file_path : all_file_paths) {
    std::cout << file_path << std::endl;
  }
  return 0;
}
