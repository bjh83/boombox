#ifndef IO_FILE_H_
#define IO_FILE_H_

#include <string>
#include "cc/utils/error.h"

namespace io {

class File {
 public:
  enum AccessMode {
    RD_ONLY,
    WR_ONLY,
    RD_WR,
  };

  static utils::ErrorOr<File> Open(const std::string& file_name,
                                   AccessMode mode);

  File(File&& file) : fd_(file.fd_) { file.fd_ = -1; }
  ~File() { Close(); }
  utils::ErrorOr<ssize_t> Read(char* buffer, size_t max_to_read);
  utils::ErrorOr<ssize_t> Write(const char* buffer, ssize_t max_to_write);
  utils::ErrorOr<size_t> Seek(size_t offset);
  utils::Error Close();

 private:
  int fd_ = -1;

  File(int fd) : fd_(fd) {}
};

} // namespace io

#endif // IO_FILE_H_
