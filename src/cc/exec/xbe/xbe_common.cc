#include "cc/exec/xbe/xbe_common.h"

using std::string;
using std::to_string;

namespace exec {
namespace xbe {
namespace {
string SectionFlagsToString(uint32_t section_flags) {
  string flags;
  if (section_flags & kSectionFlagWritableMask) {
    flags += "WRITABLE,";
  }
  if (section_flags & kSectionFlagPreloadMask) {
    flags += "PRELOAD,";
  }
  if (section_flags & kSectionFlagExecutableMask) {
    flags += "EXEC,";
  }
  if (section_flags & kSectionFlagInsertedFileMask) {
    flags += "INSERT_FILE,";
  }
  if (section_flags & kSectionFlagHeadPageReadOnly) {
    flags += "HEAD_PAGE,";
  }
  if (section_flags & kSectionFlagTailPageReadOnly) {
    flags += "TAIL_PAGE,";
  }
  if (section_flags & ~(kSectionFlagWritableMask 
                        | kSectionFlagPreloadMask
                        | kSectionFlagExecutableMask
                        | kSectionFlagInsertedFileMask
                        | kSectionFlagHeadPageReadOnly
                        | kSectionFlagTailPageReadOnly)) {
    return "ERROR!";
  }
  return flags;
}
} // namespace

string ToString(const XbeImageHeader& image_header) {
  const string magic_number(
      reinterpret_cast<const char*>(&image_header.magic_number), 4);
  return string()
      + "XbeImageHeader {\n"
      + "  magic_number = " + magic_number + "\n"
      + "  digital_signature = ...\n"
      + "  base_mem_addr = " + to_string(image_header.base_mem_addr) + "\n"
      + "  headers_size = " + to_string(image_header.headers_size) + "\n"
      + "  image_size = " + to_string(image_header.image_size) + "\n"
      + "  image_header_size = " + to_string(image_header.image_header_size) 
          + "\n"
      + "  date_time = " + to_string(image_header.date_time) + "\n"
      + "  cert_mem_addr = " + to_string(image_header.cert_mem_addr) + "\n"
      + "  section_header_num = " + to_string(image_header.section_header_num) 
          + "\n"
      + "  section_header_mem_addr = " 
          + to_string(image_header.section_header_mem_addr) + "\n"
      + "  init_flags = " + to_string(image_header.init_flags) + "\n"
      + "  entry_mem_addr = "
          + to_string(image_header.entry_mem_addr ^ kEntryMemAddrXorKey) + "\n"
      + "  tls_mem_addr = " + to_string(image_header.tls_mem_addr) + "\n"
      + "  pe_stack_commit = " + to_string(image_header.pe_stack_commit) + "\n"
      + "  pe_heap_reserve = " + to_string(image_header.pe_heap_reserve) + "\n"
      + "  pe_heap_commit = " + to_string(image_header.pe_heap_commit) + "\n"
      + "  pe_base_address = " + to_string(image_header.pe_base_address) + "\n"
      + "  pe_image_size = " + to_string(image_header.pe_image_size) + "\n"
      + "  pe_checksum = " + to_string(image_header.pe_checksum) + "\n"
      + "  pe_date_time = " + to_string(image_header.date_time) + "\n"
      + "  debug_pathname_mem_addr = " 
          + to_string(image_header.debug_pathname_mem_addr) + "\n"
      + "  debug_filename_mem_addr = " 
          + to_string(image_header.debug_filename_mem_addr) + "\n"
      + "  debug_unicode_filename_mem_addr = " 
          + to_string(image_header.debug_unicode_filename_mem_addr) + "\n"
      + "  kernel_thunk_mem_addr = " 
          + to_string(image_header.kernel_thunk_mem_addr) + "\n"
      + "  non_kernel_import_mem_addr = " 
          + to_string(image_header.non_kernel_import_mem_addr) + "\n"
      + "  library_version_num = " 
          + to_string(image_header.library_version_num) + "\n"
      + "  library_version_mem_addr = " 
          + to_string(image_header.library_version_mem_addr) + "\n"
      + "  kernel_library_version_mem_addr = " 
          + to_string(image_header.kernel_library_version_mem_addr) + "\n"
      + "  xapi_library_version_mem_addr = " 
          + to_string(image_header.xapi_library_version_mem_addr) + "\n"
      + "  logo_bitmap_addr = " + to_string(image_header.logo_bitmap_addr)
          + "\n"
      + "  logo_bitmap_size = " + to_string(image_header.logo_bitmap_size)
          + "\n"
      + "}\n";
}

string ToString(const XbeSectionHeader& section_header) {
  return string()
      + "XbeSectionHeader {\n"
      + "  section_flags = " + SectionFlagsToString(section_header.section_flags) + "\n"
      + "  virt_mem_addr = " + to_string(section_header.virt_mem_addr) + "\n"
      + "  virt_mem_size = " + to_string(section_header.virt_mem_size) + "\n"
      + "  file_offset = " + to_string(section_header.file_offset) + "\n"
      + "  file_size = " + to_string(section_header.file_size) + "\n"
      + "  sect_name_mem_addr = " + to_string(section_header.sect_name_mem_addr)
          + "\n"
      + "  sect_name_ref_count = "
          + to_string(section_header.sect_name_ref_count) + "\n"
      + "  head_shared_page_ref_count_mem_addr = "
          + to_string(section_header.head_shared_page_ref_count_mem_addr) + "\n"
      + "  tail_shared_page_ref_count_mem_addr = "
          + to_string(section_header.tail_shared_page_ref_count_mem_addr) + "\n"
      + "  section_digest = ... \n"
      + "}\n";
}

} // namespace xbe
} // namespace xbe
