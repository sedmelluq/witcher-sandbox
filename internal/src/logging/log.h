#pragma once

#define FMT_HEADER_ONLY
#include <spdlog/spdlog.h>

namespace logger {
  void setup_logger();

  std::string wide(const std::wstring& value);
  std::string wide(const wchar_t* value);

  template <typename T>
  inline uint64_t ptr(const T *p) { return (uint64_t) p; }

  extern std::shared_ptr<spdlog::logger> it;
}
