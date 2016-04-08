#ifndef EXEC_XBE_XBE_COMMON_H_
#define EXEC_XBE_XBE_COMMON_H_

#include <cstdint>
#include <string>

namespace exec {
namespace xbe {

// Same as "XBEH".
static const uint32_t kXbeMagicNumber = 0x48454258;
static const uint32_t kDigitalSignatureSize = 256;
static const uint32_t kEntryMemAddrXorKey = 0xa8fc57ab;

struct XbeImageHeader {
  uint32_t magic_number;
  uint8_t digital_signature[kDigitalSignatureSize];
  uint32_t base_mem_addr;
  uint32_t headers_size;
  uint32_t image_size;
  uint32_t image_header_size;
  uint32_t date_time;
  uint32_t cert_mem_addr;
  uint32_t section_header_num;
  uint32_t section_header_mem_addr;
  uint32_t init_flags;
  uint32_t entry_mem_addr;
  uint32_t tls_mem_addr;
  uint32_t pe_stack_commit;
  uint32_t pe_heap_reserve;
  uint32_t pe_heap_commit;
  uint32_t pe_base_address;
  uint32_t pe_image_size;
  uint32_t pe_checksum;
  uint32_t pe_date_time;
  uint32_t debug_pathname_mem_addr;
  uint32_t debug_filename_mem_addr;
  uint32_t debug_unicode_filename_mem_addr;
  uint32_t kernel_thunk_mem_addr;
  uint32_t non_kernel_import_mem_addr;
  uint32_t library_version_num;
  uint32_t library_version_mem_addr;
  uint32_t kernel_library_version_mem_addr;
  uint32_t xapi_library_version_mem_addr;
  uint32_t logo_bitmap_addr;
  uint32_t logo_bitmap_size;
};

static const uint32_t kXbeCertificateSize = 0x1d0;

static const uint32_t kSectionDigestSize = 20;
static const uint32_t kSectionFlagWritableMask = 1;
static const uint32_t kSectionFlagPreloadMask = 1 << 1;
static const uint32_t kSectionFlagExecutableMask = 1 << 2;
static const uint32_t kSectionFlagInsertedFileMask = 1 << 3;
static const uint32_t kSectionFlagHeadPageReadOnly = 1 << 4;
static const uint32_t kSectionFlagTailPageReadOnly = 1 << 5;

struct XbeSectionHeader {
  uint32_t section_flags;
  uint32_t virt_mem_addr;
  uint32_t virt_mem_size;
  uint32_t file_offset;
  uint32_t file_size;
  uint32_t sect_name_mem_addr;
  uint32_t sect_name_ref_count;
  uint32_t head_shared_page_ref_count_mem_addr;
  uint32_t tail_shared_page_ref_count_mem_addr;
  uint8_t section_digest[kSectionDigestSize];
};

std::string ToString(const XbeImageHeader& image_header);
std::string ToString(const XbeSectionHeader& section_header);

} // namespace xbe
} // namespace xbe

#endif // EXEC_XBE_XBE_COMMON_H_
