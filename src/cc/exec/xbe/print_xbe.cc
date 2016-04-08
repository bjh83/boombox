#include <iostream>
#include <vector>

#include "cc/exec/xbe/xbe_common.h"
#include "cc/io/file.h"
#include "cc/utils/error.h"

using std::cout;
using std::endl;
using std::vector;
using exec::xbe::kXbeCertificateSize;
using exec::xbe::ToString;
using exec::xbe::XbeImageHeader;
using exec::xbe::XbeSectionHeader;
using io::File;
using utils::ErrorOr;

int main(int argc, char* argv[]) {
  CHECK_INFO(argc == 2, "Must specify path to xbe.");
  ErrorOr<File> error_or_xbe_file = File::Open(argv[1], File::RD_ONLY);
  CHECK_ERROR(error_or_xbe_file.error());
  File xbe_file = error_or_xbe_file.move();

  XbeImageHeader image_header;
  CHECK_ERROR(xbe_file.Read(reinterpret_cast<char*>(&image_header),
                            sizeof(XbeImageHeader)).error());

  const size_t section_header_offset =
      image_header.section_header_mem_addr - image_header.base_mem_addr;
  CHECK_ERROR(xbe_file.Seek(section_header_offset).error());

  vector<XbeSectionHeader> section_headers(image_header.section_header_num);
  CHECK_ERROR(xbe_file.Read(reinterpret_cast<char*>(section_headers.data()),
                            sizeof(XbeSectionHeader) * section_headers.size())
              .error());

  cout << ToString(image_header) << endl;
  for (const XbeSectionHeader& section_header : section_headers) {
    cout << ToString(section_header) << endl;
  }
  return 0;
}
