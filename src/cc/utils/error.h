#ifndef UTILS_ERROR_H_
#define UTILS_ERROR_H_

#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

namespace utils {

#define RETURN_ERROR(info) return utils::Error(info, __FILE__, __LINE__);

#define RETURN_ERROR_IF(predicate, info) if (predicate) { RETURN_ERROR(info); }

#define RETURN_ERROR_IF_NOT(predicate, info) RETURN_ERROR_IF(!(predicate), info)

#define PASS_ERROR(error) \
    { utils::Error temp = error; if (!temp.is_ok()) { return error; } }

#define RETURN_ERROR_SYSCALL(result, info) \
    RETURN_ERROR_IF_NOT(result >= 0, std::string(info) + ": " + strerror(errno))

#define CHECK_INFO(predicate, message) if (!(predicate)) { FAIL(message); }

#define CHECK(predicate) CHECK_INFO(predicate, "Failed check: " #predicate)

#define CHECK_ERROR(error) CHECK_INFO(error.is_ok(), error.error_info())

#define FAIL(message) std::cout << message << std::endl; abort();

class Error {
 public:
  Error(const std::string& error_info) : error_info_(error_info) {}

  Error(const std::string& error_info,
        const std::string& file_name,
        int line_number) 
      : error_info_(file_name + ":" + std::to_string(line_number) + "] "
                    + error_info) {}

  static Error Ok() {
    Error ok("");
    ok.ok_ = true;
    return ok;
  }

  bool is_ok() const { return ok_; }
  const std::string& error_info() const { return error_info_; }

 private:
  bool ok_;
  std::string error_info_;
};

template <typename T>
class ErrorOr {
 public:
  ErrorOr(const Error& error) : error_(error) {}
  ErrorOr(T&& value) 
      : memory_ptr_(new(memory_) T(std::move(value))), error_(Error::Ok()) {}

  ~ErrorOr() { if (is_ok() && memory_ptr_) { memory_ptr_->~T(); } }

  bool is_ok() const { return error_.is_ok(); }
  const std::string& error_info() const { return error_.error_info(); }
  const Error& error() const { return error_; }
  const T& get() const { CHECK(is_ok() && memory_ptr_); return *memory_ptr_; }
  T&& move() { CHECK(is_ok() && memory_ptr_); return std::move(*memory_ptr_); }
  
 private:
  T* memory_ptr_ = nullptr;
  Error error_;

  char memory_[sizeof(T)];
};

} // namespace utils

#endif // UITLS_ERROR_H_
