#pragma once

#include <vector>
#include <cstdint>
#include "../logging/log.h"

template <class Type>
std::vector<uint8_t>& message_append(std::vector<uint8_t>& message, const Type& value) {
  auto value_array = reinterpret_cast<const uint8_t*>(&value);
  message.insert(message.end(), value_array, value_array + sizeof(value));
  return message;
}

std::vector<uint8_t>& message_append(std::vector<uint8_t>& message, const void* value, size_t length) {
  auto value_array = reinterpret_cast<const uint8_t*>(value);
  message.insert(message.end(), value_array, value_array + length);
  return message;
}

std::vector<uint8_t>& message_append_string(std::vector<uint8_t>& message, const std::string& string) {
  uint32_t length = string.length();
  message_append(message, length);
  message_append(message, string.c_str(), length);
  return message;
}

std::vector<uint8_t>& message_append_string(std::vector<uint8_t>& message, const std::wstring& string) {
  return message_append_string(message, logger::wide(string));
}
