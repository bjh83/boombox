#ifndef EXEC_ELF_ELF_H_
#define EXEC_ELF_ELF_H_

#include "cc/io/file.h"
#include "cc/utils/error.h"

namespace exec {
namespace elf {

utils::Error MakeElfFromXbe(io::File* xbe_file, io::File* elf_file);

} // namespace elf
} // namespace exec

#endif // EXEC_ELF_ELF_H_
