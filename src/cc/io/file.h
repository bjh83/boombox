#ifndef IO_FILE_H_
#define IO_FILE_H_

#include <string>
#include "cc/io/file_like.h"
#include "cc/utils/error.h"

namespace io {

class File : public FileLike {
 public:
  enum AccessMode {
    RD_ONLY,
    WR_ONLY,
    RD_WR,
  };

  static utils::ErrorOr<File> Create(const std::string& file_name,
                                     int permissions);

  static utils::ErrorOr<File> Open(const std::string& file_name,
                                   AccessMode mode);

  File(File&& file) : fd_(file.fd_) { file.fd_ = -1; }
  ~File() { Close(); }
  utils::ErrorOr<ssize_t> Read(char* buffer, size_t max_to_read) override;
  utils::ErrorOr<ssize_t> Write(const char* buffer,
                                size_t max_to_write) override;
  utils::ErrorOr<size_t> Seek(size_t offset) override;
  utils::Error Close() override;

 private:
  int fd_ = -1;

  File(int fd) : fd_(fd) {}

  File(const File&) = delete;
  File& operator=(const File&) = delete;
};

} // namespace io

#endif // IO_FILE_H_
