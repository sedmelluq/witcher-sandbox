#pragma once

#include <cstdint>
#include "../windows_api.h"
#include "wrapper_address_space.h"
#include "modifiable_code.h"

class ExecutableAddressSpace {
public:
  ExecutableAddressSpace() {
    base_address = (uint64_t) GetModuleHandleW(nullptr);
  }

  uint64_t by_offset(uint64_t offset) {
    return base_address + offset;
  }

  ModifiableCode modify_code(uint64_t offset, uint64_t length) {
    return ModifiableCode(base_address + offset, length);
  }

  WrapperAddressSpace create_wrapper_space() {
    for (uint64_t i = 1; i < 0x200; i++) {
      uint64_t offset = i * 0x10000;

      if (offset >= base_address) {
        break;
      }

      void* result = VirtualAlloc((void*) (base_address - offset), 0x10000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READ);

      if (result != nullptr) {
        return WrapperAddressSpace((uint64_t) result, 0x10000);
      }
    }

    throw std::exception("Unable to find vacant memory before executable region for wrappers.");
  }

  uint64_t base_address;
};
