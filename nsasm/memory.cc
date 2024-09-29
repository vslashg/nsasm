#include "nsasm/memory.h"

#include "nsasm/error.h"

namespace nsasm {

ErrorOr<int> InputSource::ReadByte(nsasm::Address address) const {
  auto read = Read(address, 1);
  NSASM_RETURN_IF_ERROR(read);
  return (*read)[0];
}

ErrorOr<int> InputSource::ReadWord(nsasm::Address address) const {
  auto read = Read(address, 2);
  NSASM_RETURN_IF_ERROR(read);
  return (*read)[0] + ((*read)[1] * 256);
}

ErrorOr<int> InputSource::ReadLong(nsasm::Address address) const {
  auto read = Read(address, 3);
  NSASM_RETURN_IF_ERROR(read);
  return (*read)[0] + ((*read)[1] * 256) + ((*read)[2] * 256 * 256);
}

}  // namespace nsasm