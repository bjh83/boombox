#ifndef IO_FILE_LIKE_H_
#define IO_FILE_LIKE_H_

#include "cc/utils/error.h"

namespace io {

class FileLike {
 public:
  virtual utils::ErrorOr<ssize_t> Read(char* buffer, size_t max_to_read) = 0;
  virtual utils::ErrorOr<ssize_t> Write(const char* buffer,
                                        size_t max_to_write) = 0;
  virtual utils::ErrorOr<size_t> Seek(size_t offset) = 0;
  virtual utils::Error Close() = 0;
};

} // namespace io

#endif // IO_FILE_LIKE_H_
