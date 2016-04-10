#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
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
using std::to_string;
using exec::elf::MakeElfFromXbe;
using io::File;
using utils::Error;
using utils::ErrorOr;

#define M_OFFSETOF(STRUCT, ELEMENT) \
      (unsigned long) &((STRUCT *)NULL)->ELEMENT;

static const uint8_t kInt3 = 0xcc;
static const uint32_t kRegsTraceFlag = 1 << 8;

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

Error DumpMem(const string& dump_path, const pid_t pid) {
  const string mem_path = string("/proc/") + to_string(pid) + "/mem";
  ErrorOr<File> error_or_mem_file = File::Open(mem_path, File::RD_ONLY);
  PASS_ERROR(error_or_mem_file.error());
  File mem_file = error_or_mem_file.move();

  ErrorOr<File> error_or_dump_file = File::Create(dump_path, 0664);
  PASS_ERROR(error_or_dump_file.error());
  File dump_file = error_or_dump_file.move();

  const size_t buffer_size = 2048;
  char buffer[buffer_size];
  ssize_t amount_read = buffer_size;
  PASS_ERROR(mem_file.Seek(0x12000).error());
  PASS_ERROR(dump_file.Seek(0x12000).error());
  while (amount_read == buffer_size) {
    ErrorOr<ssize_t> error_or_amount_read = mem_file.Read(buffer, buffer_size);
    PASS_ERROR(error_or_amount_read.error());
    amount_read = error_or_amount_read.get();
    PASS_ERROR(dump_file.Write(buffer, amount_read).error());
  }
  return Error::Ok();
}

Error WatchExec(const pid_t child_pid, const string& dump_path) {
  int status;
  // Wait for child to call execve.
  cout << "About to wait for child: pid " << child_pid << endl;
  RETURN_ERROR_SYSCALL(waitpid(child_pid, &status, WSTOPPED), "Wait failed.");
  if (!WIFSTOPPED(status)) {
    RETURN_ERROR("Program did not stop.");
  }

  // PASS_ERROR(DumpMem(dump_path, child_pid));

  int cmd;
  while ((cmd = getchar()) != 'q') {
    RETURN_ERROR_SYSCALL(ptrace(PTRACE_SINGLESTEP, child_pid, nullptr, 0),
                         "Inserting sigtrap failed.");

    RETURN_ERROR_SYSCALL(waitpid(child_pid, &status, WSTOPPED), "Wait failed.");
    if (!WIFSTOPPED(status)) {
      RETURN_ERROR("Program did not stop.");
    }

    user_regs_struct regs;
    RETURN_ERROR_SYSCALL(ptrace(PTRACE_GETREGS, child_pid, 0, &regs),
                         "Could not read regs.");
    cout << "Current rip: 0x" << std::hex << regs.rip << endl;
  }
  // int opcode = ptrace(PTRACE_PEEKTEXT, child_pid, 0xf48d5, 0);
  // RETURN_ERROR_SYSCALL(opcode,
  //                      "Could not read text.");
  // cout << "Read opcode: " << std::hex << opcode << endl;
  // cout << "Continuing child ..." << endl;
  // RETURN_ERROR_SYSCALL(ptrace(PTRACE_CONT, child_pid, nullptr, nullptr),
  //                      "Could not continue child.");

  // RETURN_ERROR_SYSCALL(waitpid(child_pid, &status, WSTOPPED), "Wait failed.");
  // if (!WIFSTOPPED(status)) {
  //   RETURN_ERROR("Program did not stop.");
  // }
  // cout << "Caught sigtrap." << endl;

  // cout << "Continuing child ..." << endl;
  // RETURN_ERROR_SYSCALL(ptrace(PTRACE_CONT, child_pid, nullptr, nullptr),
  //                      "Could not continue child.");

  // RETURN_ERROR_SYSCALL(waitpid(child_pid, &status, WSTOPPED), "Wait failed.");
  // if (!WIFSTOPPED(status)) {
  //   RETURN_ERROR("Program did not stop.");
  // }
  // cout << "Caught sigtrap." << endl;

  // user_regs_struct regs;
  // RETURN_ERROR_SYSCALL(ptrace(PTRACE_GETREGS, child_pid, 0, &regs),
  //                      "Could not read regs.");
  // cout << "Current rip: 0x" << std::hex << regs.rip << endl;

  // Wait for child death.
  RETURN_ERROR_SYSCALL(waitpid(child_pid, &status, WEXITED), "Wait failed.");
  cout << "Child died with status: " << status << endl;
  return Error::Ok();
}

Error ExecElf(const string& elf_path, const string& dump_path) {
  pid_t pid = fork();
  RETURN_ERROR_SYSCALL(pid, "Could not fork.");
  if (pid) {
    PASS_ERROR(WatchExec(pid, dump_path));
  } else {
    PASS_ERROR(InitExec(elf_path));
  }
  return Error::Ok();
}

int main(int argc, char* argv[]) {
  CHECK_INFO(argc == 2, "Must specify path to xbe.");
  const string xbe_path = argv[1];
  const string elf_path = xbe_path + ".bin";
  const string dump_path = xbe_path + ".memdump";

  ErrorOr<File> error_or_xbe_file = File::Open(xbe_path, File::RD_ONLY);
  CHECK_ERROR(error_or_xbe_file.error());
  File xbe_file = error_or_xbe_file.move();

  ErrorOr<File> error_or_elf_file = File::Create(elf_path, 0777);
  CHECK_ERROR(error_or_elf_file.error());
  File elf_file = error_or_elf_file.move();

  CHECK_ERROR(MakeElfFromXbe(&xbe_file, &elf_file));
  CHECK_ERROR(xbe_file.Close());
  CHECK_ERROR(elf_file.Close());

  CHECK_ERROR(ExecElf(elf_path, dump_path));

  return 0;
}
