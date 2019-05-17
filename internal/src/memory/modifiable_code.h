#pragma once

#include <cstdint>
#include <exception>
#include "../windows_api.h"

class ModifiableCode {
public:
  ModifiableCode(uint64_t address, uint64_t length) {
    this->address = (uint8_t*) address;
    this->length = length;
    this->write_offset = 0;

    VirtualProtect((void*) this->address, this->length, PAGE_READWRITE, (DWORD*) &previous_protection);
  }

  ModifiableCode(ModifiableCode&& other) noexcept {
    address = other.address;
    length = other.length;
    previous_protection = other.previous_protection;
    write_offset = other.write_offset;

    other.address = nullptr;
  }

  ~ModifiableCode() {
    if (this->address != nullptr) {
      VirtualProtect((void *) this->address, this->length, previous_protection, (DWORD *) &previous_protection);
    }
  }

  void bytes(std::initializer_list<uint8_t> bytes) {
    check_writable(bytes.size());

    for (uint8_t byte : bytes) {
      address[write_offset++] = byte;
    }
  }

  void u32(uint32_t value) {
    check_writable(sizeof(uint32_t));

    *(uint32_t*) (&address[write_offset]) = value;
    write_offset += sizeof(uint32_t);
  }

  void u64(uint64_t value) {
    check_writable(sizeof(uint64_t));

    *(uint64_t*) (&address[write_offset]) = value;
    write_offset += sizeof(uint64_t);
  }

  void offset32(uint64_t target) {
    uint64_t source = ((uint64_t) address) + write_offset + 4;
    u32((uint32_t) (target - source));
  }

  uint8_t* address;

private:
  void check_writable(size_t byte_count) {
    if (byte_count + write_offset > length) {
      throw std::exception("Writing out of bounds of modifiable region.");
    }
  }

  uint64_t length;
  uint32_t previous_protection {};
  uint64_t write_offset;
};
