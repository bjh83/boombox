#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <vector>

#include "cc/io/file.h"
#include "cc/io/xdfs/xdfs.h"
#include "cc/io/xdfs/xdfs_dir.h"
#include "cc/io/xdfs/xdfs_file.h"
#include "cc/utils/error.h"

using std::string;
using std::vector;
using io::File;
using io::xdfs::IsDir;
using io::xdfs::Xdfs;
using io::xdfs::XdfsDir;
using io::xdfs::XdfsDirEntry;
using io::xdfs::XdfsFile;
using utils::Error;
using utils::ErrorOr;

struct FilePathsAndDirPaths {
  vector<string> dir_paths;
  vector<string> file_paths;
};

FilePathsAndDirPaths FindAllFilePaths(Xdfs* xdfs) {
  FilePathsAndDirPaths paths;
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
        paths.dir_paths.push_back(new_path);
      } else {
        paths.file_paths.push_back(new_path);
      }
    }
  }
  return paths;
}

Error MakeDirs(const string& root_dir, const vector<string>& dirs) {
  for (const string& dir : dirs) {
    RETURN_ERROR_SYSCALL(mkdir((root_dir + dir).c_str(), 0775), "");
  }
  return Error::Ok();
} 

Error CopyFileFromTo(XdfsFile* xdfs_file, File* local_file) {
  const size_t buffer_size = 2048;
  char buffer[buffer_size];
  ssize_t amount_read = buffer_size;
  while (amount_read == buffer_size) {
    ErrorOr<ssize_t> error_or_amount_read =
        xdfs_file->Read(buffer, buffer_size);
    PASS_ERROR(error_or_amount_read.error());
    amount_read = error_or_amount_read.get();
    PASS_ERROR(local_file->Write(buffer, amount_read).error());
  }
  return Error::Ok();
}

Error CopyFiles(Xdfs* xdfs, const string& root_dir, const vector<string>& xdfs_dirs) {
  for (const string& xdfs_path : xdfs_dirs) {
    std::cout << "Extracting file " << xdfs_path << " ..." << std::endl;
    ErrorOr<File> error_or_local_file
        = File::Create(root_dir + xdfs_path, 0664);
    PASS_ERROR(error_or_local_file.error());
    ErrorOr<XdfsFile> error_or_xdfs_file = xdfs->OpenFile(xdfs_path);
    PASS_ERROR(error_or_xdfs_file.error());
    File local_file = error_or_local_file.move();
    XdfsFile xdfs_file = error_or_xdfs_file.move();
    PASS_ERROR(CopyFileFromTo(&xdfs_file, &local_file));
  }
  return Error::Ok();
}

Error ExtractFromIso(File&& iso_file, const string& dir_extract_to) {
  ErrorOr<Xdfs> error_or_xdfs = Xdfs::CreateXdfs(std::move(iso_file));
  PASS_ERROR(error_or_xdfs.error());
  FilePathsAndDirPaths file_paths_and_dir_paths =
      FindAllFilePaths(error_or_xdfs.mutable_ptr());
  PASS_ERROR(MakeDirs(dir_extract_to, file_paths_and_dir_paths.dir_paths));
  PASS_ERROR(CopyFiles(error_or_xdfs.mutable_ptr(),
                       dir_extract_to,
                       file_paths_and_dir_paths.file_paths));
  std::cout << "Extracting files complete." << std::endl;
  return Error::Ok();
}

int main(int argc, char* argv[]) {
  CHECK_INFO(argc == 3,
             "Path to ISO and directory to extract to must be provided.");
  ErrorOr<File> error_or_iso_file = File::Open(argv[1], File::RD_ONLY);
  CHECK_ERROR(error_or_iso_file.error());
  CHECK_ERROR(ExtractFromIso(error_or_iso_file.move(), argv[2]));
  return 0;
}
