#pragma once

#include <cstdint>
#include "modifiable_code.h"

class WrapperAddressSpace {
public:
  WrapperAddressSpace(uint64_t base_address, size_t length) {
    this->base_address = base_address;
    this->unused_offset = 0;
    this->length = length;
  }

  WrapperAddressSpace(WrapperAddressSpace&& other) noexcept {
    base_address = other.base_address;
    unused_offset = other.unused_offset;
    length = other.length;

    other.base_address = 0;
  }

  ~WrapperAddressSpace() {
    if (base_address != 0) {
      VirtualFree((void*) base_address, 0, MEM_RELEASE);
      base_address = 0;
    }
  }

  ModifiableCode reserve(size_t segment_length) {
    if (base_address == 0 || unused_offset + segment_length > length) {
      throw std::exception("Wrapper address space is exhausted");
    }

    ModifiableCode code(base_address + unused_offset, segment_length);
    unused_offset += segment_length;
    return code;
  }

private:
  uint64_t base_address;
  size_t unused_offset;
  size_t length;
};
