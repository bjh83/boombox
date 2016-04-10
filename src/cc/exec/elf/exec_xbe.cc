#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "cc/exec/elf/elf.h"
#include "cc/io/file.h"
#include "cc/utils/error.h"

using std::cout;
using std::endl;
using std::string;
using exec::elf::MakeElfFromXbe;
using io::File;
using utils::Error;
using utils::ErrorOr;

Error InitExec(const string& elf_path) {
  RETURN_ERROR_SYSCALL(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr),
                       "Could not request trace.");
  char* const argv[2] = {(char* const) elf_path.c_str(), nullptr};
  char* const envp[1] = {nullptr};
  cout << "About to exec elf ..." << endl;
  RETURN_ERROR_SYSCALL(execve(elf_path.c_str(), argv, envp),
                       "Could not execute elf.");
  return Error::Ok();
}

Error WatchExec(const pid_t child_pid) {
  int status;
  // Wait for child to call execve.
  RETURN_ERROR_SYSCALL(waitpid(child_pid, &status, __WALL), "Wait failed.");
  if (!WIFSTOPPED(status)) {
    RETURN_ERROR("Program did not stop.");
  }

  cout << "Continuing child ..." << endl;
  RETURN_ERROR_SYSCALL(ptrace(PTRACE_CONT, 0, nullptr, nullptr),
                       "Could not continue child.");


  // Wait for child death.
  RETURN_ERROR_SYSCALL(waitpid(child_pid, &status, WEXITED), "Wait failed.");
  cout << "Child died with status: " << status << endl;
  return Error::Ok();
}

Error ExecElf(const string& elf_path) {
  pid_t pid = fork();
  RETURN_ERROR_SYSCALL(pid, "Could not fork.");
  if (pid) {
    WatchExec(pid);
  } else {
    InitExec(elf_path);
  }
  return Error::Ok();
}

int main(int argc, char* argv[]) {
  CHECK_INFO(argc == 2, "Must specify path to xbe.");
  const string xbe_path = argv[1];

  ErrorOr<File> error_or_xbe_file = File::Open(xbe_path, File::RD_ONLY);
  CHECK_ERROR(error_or_xbe_file.error());
  File xbe_file = error_or_xbe_file.move();

  const string elf_path = xbe_path + ".bin";
  ErrorOr<File> error_or_elf_file = File::Create(elf_path, 0777);
  CHECK_ERROR(error_or_elf_file.error());
  File elf_file = error_or_elf_file.move();

  CHECK_ERROR(MakeElfFromXbe(&xbe_file, &elf_file));
  CHECK_ERROR(xbe_file.Close());
  CHECK_ERROR(elf_file.Close());

  CHECK_ERROR(ExecElf(elf_path));

  return 0;
}
