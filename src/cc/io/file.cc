#include "cc/io/file.h"

#include <fcntl.h>
#include <unistd.h>

using std::string;
using utils::Error;
using utils::ErrorOr;

namespace io {

namespace {
int ToNativeAccessMode(File::AccessMode access_mode) {
  switch (access_mode) {
    case File::RD_ONLY:
      return O_RDONLY;
    case File::WR_ONLY:
      return O_WRONLY;
    case File::RD_WR:
      return O_RDWR;
    default:
      FAIL("Invalid access mode given.");
  }
}
} // namespace

ErrorOr<File> File::Open(const string& file_name, AccessMode access_mode) {
  int fd = open(file_name.c_str(), ToNativeAccessMode(access_mode));
  RETURN_ERROR_SYSCALL(fd, "Could not open file.");
  return ErrorOr<File>(File(fd));
}

ErrorOr<ssize_t> File::Read(char* buffer, size_t max_to_read) {
  CHECK(fd_ >= 0);
  ssize_t amount_did_read = read(fd_, buffer, max_to_read);
  RETURN_ERROR_SYSCALL(amount_did_read, "Reading file failed.");
  return ErrorOr<ssize_t>(std::move(amount_did_read));
}

ErrorOr<ssize_t> File::Write(const char* buffer, ssize_t max_to_write) {
  CHECK(fd_ >= 0);
  ssize_t amount_did_write = write(fd_, buffer, max_to_write);
  RETURN_ERROR_SYSCALL(amount_did_write, "Writing file failed.");
  return ErrorOr<ssize_t>(std::move(amount_did_write));
}

ErrorOr<size_t> File::Seek(size_t offset) {
  CHECK(fd_ >= 0);
  ssize_t new_offset = lseek(fd_, offset, SEEK_SET);
  RETURN_ERROR_SYSCALL(new_offset, "Seeking file failed.");
  return ErrorOr<size_t>(std::move(new_offset));
}

Error File::Close() {
  if (fd_ >= 0) {
    RETURN_ERROR_SYSCALL(close(fd_), "Could not close file.");
  }
  return Error::Ok();
}

} // namespace io
