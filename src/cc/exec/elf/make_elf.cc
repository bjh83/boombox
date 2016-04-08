#include <string>

#include "cc/exec/elf/elf.h"
#include "cc/io/file.h"
#include "cc/utils/error.h"

using std::string;
using exec::elf::MakeElfFromXbe;
using io::File;
using utils::ErrorOr;

int main(int argc, char* argv[]) {
  CHECK_INFO(argc == 2, "Must specify path to xbe.");
  const string xbe_path = argv[1];

  ErrorOr<File> error_or_xbe_file = File::Open(xbe_path, File::RD_ONLY);
  CHECK_ERROR(error_or_xbe_file.error());
  File xbe_file = error_or_xbe_file.move();

  ErrorOr<File> error_or_elf_file = File::Create(xbe_path + ".bin", 0777);
  CHECK_ERROR(error_or_elf_file.error());
  File elf_file = error_or_elf_file.move();

  CHECK_ERROR(MakeElfFromXbe(&xbe_file, &elf_file));

  return 0;
}
