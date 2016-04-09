#include "cc/exec/elf/elf.h"

#include <elf.h>
#include <map>
#include <vector>
#include "cc/exec/xbe/xbe_common.h"

using std::map;
using std::string;
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
    .e_machine = EM_386,
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

size_t SegNumAndSecNumToOffset(const int segment_number,
                               const int section_number) {
  return SegNumToOffset(segment_number) + sizeof(Elf32_Shdr) * section_number;
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

uint32_t XbeToElfShdrFlags(const uint32_t xbe_flags) {
  uint32_t sh_flags = SHF_ALLOC;
  if (xbe_flags & kSectionFlagWritableMask) {
    sh_flags |= SHF_WRITE;
  }
  if (xbe_flags & kSectionFlagExecutableMask) {
    sh_flags |= SHF_EXECINSTR;
  }
  return sh_flags;
}

Elf32_Shdr MakeElfNullSectionHeader() {
  Elf32_Shdr header = {
    .sh_name = 0,
    .sh_type = SHT_NULL,
    .sh_flags = 0,
    .sh_addr = 0,
    .sh_offset = 0,
    .sh_size = 0,
    .sh_link = 0,
    .sh_info = 0,
    .sh_addralign = 0,
    .sh_entsize = 0,
  };
  return header;
}

Elf32_Shdr MakeElfSectionHeader(const XbeSectionHeader& xbe_section_header,
                                const map<uint32_t, uint32_t>& mem_addr_to_index
                                ) {
  Elf32_Shdr header = {
    .sh_name = mem_addr_to_index.at(xbe_section_header.sect_name_mem_addr),
    .sh_type = SHT_PROGBITS,
    .sh_flags = XbeToElfShdrFlags(xbe_section_header.section_flags),
    .sh_addr = xbe_section_header.virt_mem_addr,
    .sh_offset = xbe_section_header.file_offset,
    .sh_size = xbe_section_header.file_size,
    .sh_link = 0,
    .sh_info = 0,
    .sh_addralign = 0,
    .sh_entsize = 0,
  };
  return header;
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

Elf32_Shdr MakeElfStrTableSectionHeader(const uint32_t str_table_offset,
                                        const uint32_t str_table_size,
                                        const uint32_t name_entry_num) {
  Elf32_Shdr header = {
    .sh_name = name_entry_num,
    .sh_type = SHT_STRTAB,
    .sh_flags = 0,
    .sh_addr = 0,
    .sh_offset = str_table_offset,
    .sh_size = str_table_size,
    .sh_link = 0,
    .sh_info = 0,
    .sh_addralign = 0,
    .sh_entsize = 0,
  };
  return header;
}

ErrorOr<map<uint32_t, string>> ReadShdrStrTable(
    File* xbe_file,
    const vector<uint32_t>& offsets) {
  map<uint32_t, string> str_table;
  for (const uint32_t start_offset : offsets) {
    PASS_ERROR(xbe_file->Seek(start_offset).error());
    string current_str;
    for (uint32_t current_offset = start_offset;; current_offset++) {
      char current_char;
      PASS_ERROR(xbe_file->Read(&current_char, 1).error());
      if (current_char == '\0') {
        break;
      }
      current_str += current_char;
    }
    str_table[start_offset] = current_str;
  }
  return ErrorOr<map<uint32_t, string>>(std::move(str_table));
}

ErrorOr<map<uint32_t, uint32_t>> CopyShdrStrTableFromXbeToElf(
    File* xbe_file,
    File* elf_file,
    const vector<uint32_t>& offsets,
    const int segment_number,
    const int section_number,
    const uint32_t offset_to_mem_addr_offset)
{
  ErrorOr<map<uint32_t, string>> error_or_shdr_str_table =
      ReadShdrStrTable(xbe_file, offsets);
  PASS_ERROR(error_or_shdr_str_table.error());

  map<uint32_t, string> shdr_str_table = error_or_shdr_str_table.move();
  const uint32_t shdr_str_table_name_index = shdr_str_table.size();
  shdr_str_table[0xffffffff] = ".shstrtab";
  const uint32_t shdr_offset = SegNumAndSecNumToOffset(segment_number,
                                                     section_number);
  const uint32_t str_table_offset = shdr_offset + sizeof(Elf32_Shdr);

  map<uint32_t, uint32_t> mem_addr_to_index;
  uint32_t str_table_size = 0;
  // uint32_t current_index = 0;
  const char null_char = '\0';
  PASS_ERROR(elf_file->Seek(str_table_offset).error());
  for (const auto& str_table_entry : shdr_str_table) {
    mem_addr_to_index[str_table_entry.first + offset_to_mem_addr_offset] =
        str_table_size + 1;

    PASS_ERROR(elf_file->Write(&null_char, 1).error());
    PASS_ERROR(elf_file->Write(str_table_entry.second.data(),
                               str_table_entry.second.size()).error());
    PASS_ERROR(elf_file->Write(&null_char, 1).error());
    str_table_size += str_table_entry.second.size() + 2;
  }

  const Elf32_Shdr shdr = MakeElfStrTableSectionHeader(str_table_offset,
                                                 str_table_size,
                                                 shdr_str_table_name_index);
  PASS_ERROR(elf_file->Seek(shdr_offset).error());
  PASS_ERROR(elf_file->Write(reinterpret_cast<const char*>(&shdr),
                             sizeof(Elf32_Shdr)).error());

  return ErrorOr<map<uint32_t, uint32_t>>(std::move(mem_addr_to_index));
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
  vector<uint32_t> shdr_name_offsets;
  for (const XbeSectionHeader& section_header : section_headers) {
    shdr_name_offsets.push_back(
        section_header.sect_name_mem_addr - image_header.base_mem_addr);
  }

  ErrorOr<map<uint32_t, uint32_t>> error_or_mem_addr_to_index =
      CopyShdrStrTableFromXbeToElf(xbe_file,
                                   elf_file,
                                   shdr_name_offsets,
                                   image_header.section_header_num,
                                   image_header.section_header_num + 1,
                                   image_header.base_mem_addr);
  PASS_ERROR(error_or_mem_addr_to_index.error());

  map<uint32_t, uint32_t> mem_addr_to_index = error_or_mem_addr_to_index.move();
  const uint32_t section_header_num = image_header.section_header_num + 2;
  const Elf32_Ehdr ehdr = MakeElfHeader(image_header.entry_mem_addr,
                                        sizeof(Elf32_Ehdr),
                                        SegNumToOffset(
                                            image_header.section_header_num),
                                        image_header.section_header_num,
                                        section_header_num,
                                        section_header_num - 1);

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

  PASS_ERROR(elf_file->Seek(SegNumToOffset(segment_number)).error());
  const Elf32_Shdr null_shdr = MakeElfNullSectionHeader();
  PASS_ERROR(elf_file->Write(reinterpret_cast<const char*>(&null_shdr),
                             sizeof(Elf32_Shdr)).error());
  for (const XbeSectionHeader& section_header : section_headers) {
    const Elf32_Shdr shdr = MakeElfSectionHeader(section_header, mem_addr_to_index);
    PASS_ERROR(elf_file->Write(reinterpret_cast<const char*>(&shdr),
                               sizeof(Elf32_Shdr)).error());
  }
  return Error::Ok();
}

} // namespace elf
} // namespace exec
