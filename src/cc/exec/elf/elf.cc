#include "cc/exec/elf/elf.h"

#include <elf.h>
#include <vector>
#include "cc/exec/xbe/xbe_common.h"

using std::vector;
using exec::xbe::kEntryMemAddrXorKey;
using exec::xbe::kSectionFlagExecutableMask;
using exec::xbe::kSectionFlagWritableMask;
using exec::xbe::XbeImageHeader;
using exec::xbe::XbeSectionHeader;
using io::File;
using utils::Error;
using utils::ErrorOr;

namespace exec {
namespace elf {

static const Elf32_Addr kEntryAddress = 0x10000;

namespace {

Elf32_Ehdr MakeElfHeader(const Elf32_Addr entry_address,
                         const Elf32_Off program_header_offset,
                         const Elf32_Off section_header_offset,
                         const uint16_t program_header_number,
                         const uint16_t section_header_number,
                         const uint16_t section_header_name_table_index) {
  Elf32_Ehdr header = {
    .e_ident = {
      ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, // Specifies that this is an ELF.
      ELFCLASS32, // We are using 32 bit format.
      ELFDATA2LSB, // We are LSB.
      EV_CURRENT, // Current version (no other valid version).
      ELFOSABI_SYSV, // UNIX System V ABI: this is what gcc does by default.
      0, // This should be 0 according to "man 5 elf".
      // Rest is padding.
    },
    .e_type = ET_EXEC,
    .e_machine = EM_X86_64,
    .e_version = EV_CURRENT,
    .e_entry = entry_address,
    .e_phoff = program_header_offset,
    .e_shoff = section_header_offset,
    .e_flags = 0, // No flags are defined for x86.
    .e_ehsize = sizeof(Elf32_Ehdr),
    .e_phentsize = sizeof(Elf32_Phdr),
    .e_phnum = program_header_number,
    .e_shentsize = sizeof(Elf32_Shdr),
    .e_shnum = section_header_number,
    .e_shstrndx = section_header_name_table_index
  };
  return header;
}

uint32_t XbeSectionFlagsToPhdrFlags(const uint32_t xbe_flags) {
  uint32_t phdr_flags = PF_R; // All XBE sections are readable.
  if (xbe_flags & kSectionFlagWritableMask) {
    phdr_flags |= PF_W;
  }
  if (xbe_flags & kSectionFlagExecutableMask) {
    phdr_flags |= PF_X;
  }
  return phdr_flags;
}

Elf32_Phdr MakeElfProgramHeader(const XbeSectionHeader& xbe_section_header) {
  Elf32_Phdr header = {
    .p_type = PT_LOAD,
    .p_offset = xbe_section_header.file_offset,
    .p_vaddr = xbe_section_header.virt_mem_addr,
    .p_paddr = 0, // Not using phys addresses.
    .p_filesz = xbe_section_header.file_size,
    .p_memsz = xbe_section_header.virt_mem_size,
    .p_flags = XbeSectionFlagsToPhdrFlags(xbe_section_header.section_flags),
    .p_align = 0, // No alignment required.
  };
  return header;
}

size_t SegNumToOffset(const int segment_number) {
  return sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr) * segment_number;
}

Error CopySegmentFromXbeToElf(File* xbe_file,
                              File* elf_file,
                              const XbeSectionHeader& xbe_section_header,
                              const int segment_number) {
  const Elf32_Phdr phdr = MakeElfProgramHeader(xbe_section_header);
  PASS_ERROR(elf_file->Seek(SegNumToOffset(segment_number)).error());
  PASS_ERROR(
      elf_file->Write(reinterpret_cast<const char*>(&phdr), sizeof(Elf32_Phdr))
      .error());

  const size_t buffer_size = 2048;
  char buffer[buffer_size];
  size_t amount_written;
  PASS_ERROR(xbe_file->Seek(phdr.p_offset).error());
  PASS_ERROR(elf_file->Seek(phdr.p_offset).error());
  for (amount_written = 0;
       amount_written + buffer_size < phdr.p_filesz;
       amount_written += buffer_size) {
    PASS_ERROR(xbe_file->Read(buffer, buffer_size).error());
    PASS_ERROR(elf_file->Write(buffer, buffer_size).error());
  }
  const size_t remainder = phdr.p_filesz - amount_written;
  PASS_ERROR(xbe_file->Read(buffer, remainder).error());
  PASS_ERROR(elf_file->Write(buffer, remainder).error());
  return Error::Ok();
}

Error ReserveAndZeroElf(File* elf_file, size_t size) {
  const size_t buffer_size = 2048;
  vector<char> buffer(buffer_size, 0);
  size_t amount_written;
  PASS_ERROR(elf_file->Seek(0).error());
  for (amount_written = 0;
       amount_written + buffer_size < size;
       amount_written += buffer_size) {
    PASS_ERROR(elf_file->Write(buffer.data(), buffer_size).error());
  }
  const size_t remainder = size - amount_written;
  PASS_ERROR(elf_file->Write(buffer.data(), remainder).error());
  return Error::Ok();
}

} // namespace

Error MakeElfFromXbe(File* xbe_file, File* elf_file) {
  XbeImageHeader image_header;
  PASS_ERROR(xbe_file->Read(reinterpret_cast<char*>(&image_header),
                            sizeof(XbeImageHeader)).error());
  // This is called security.
  image_header.entry_mem_addr
      = image_header.entry_mem_addr ^ kEntryMemAddrXorKey;

  const size_t section_header_offset =
      image_header.section_header_mem_addr - image_header.base_mem_addr;
  PASS_ERROR(xbe_file->Seek(section_header_offset).error());

  vector<XbeSectionHeader> section_headers(image_header.section_header_num);
  PASS_ERROR(xbe_file->Read(reinterpret_cast<char*>(section_headers.data()),
                            sizeof(XbeSectionHeader) * section_headers.size())
             .error());

  const Elf32_Ehdr ehdr = MakeElfHeader(image_header.entry_mem_addr,
                                        sizeof(Elf32_Ehdr),
                                        0, // We don't have sections.
                                        image_header.section_header_num,
                                        0, // We don't have sections.
                                        SHN_UNDEF);

  // ErrorOr<size_t> error_or_file_size = xbe_file->Seek(INT32_MAX);
  // PASS_ERROR(error_or_file_size.error());
  // size_t file_size = error_or_file_size.get();
  // PASS_ERROR(ReserveAndZeroElf(elf_file, file_size));

  PASS_ERROR(elf_file->Seek(0).error());
  PASS_ERROR(
      elf_file->Write(reinterpret_cast<const char*>(&ehdr), sizeof(Elf32_Ehdr))
      .error());

  int segment_number = 0;
  for (const XbeSectionHeader& section_header : section_headers) {
    PASS_ERROR(CopySegmentFromXbeToElf(xbe_file,
                                       elf_file,
                                       section_header,
                                       segment_number++));
  }
  return Error::Ok();
}

} // namespace elf
} // namespace exec
