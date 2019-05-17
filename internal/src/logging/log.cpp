#include "log.h"

#include "../windows_api.h"
#include <Psapi.h>
#include <filesystem>
#include <time.h>

namespace fs = std::experimental::filesystem;

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/async.h>

namespace logger {
  static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> wide_converter;

  std::string wide(const std::wstring& value) {
    return std::string(wide_converter.to_bytes(value));
  }

  std::string wide(const wchar_t* value) {
    return std::string(wide_converter.to_bytes(value));
  }

  static void setup_logger_throw() {
    HMODULE module;
    wchar_t dll_path_raw[MAX_PATH];

    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR) &it, &module)) {
      throw std::exception("Could not get self module.");
    } else if (GetModuleFileNameW(module, dll_path_raw, sizeof(dll_path_raw)) == 0) {
      throw std::exception("Could not get self module path.");
    }

    fs::path path(dll_path_raw);
    fs::path log_directory = fs::absolute(path.parent_path().parent_path() / "log");

    std::error_code creation_error;

    if (!fs::create_directories(log_directory, creation_error)) {
      if (creation_error.value() != 0) {
        throw std::exception("Failed to create logs directory.");
      }
    }

    fs::path log_file = log_directory / ("debug_" + std::to_string(time(nullptr)) + ".log");

    auto currentLog = spdlog::create_async_nb<spdlog::sinks::basic_file_sink_mt>("main", log_file.string());

    spdlog::flush_every(std::chrono::seconds(2));

    currentLog->flush_on(spdlog::level::warn);
    currentLog->set_level(spdlog::level::trace);
    it = std::move(currentLog);
  }

  void setup_logger() {
    try {
      setup_logger_throw();
    } catch (const std::exception& e) {
      MessageBoxA(nullptr, e.what(), "Log initialization error.", MB_OK);
      it->error("Log initialization failed: {}", e.what());
    }
  }

  static std::shared_ptr<spdlog::logger> primordial_logger() {
    return spdlog::stdout_logger_mt("primordial");
  }

  std::shared_ptr<spdlog::logger> it = primordial_logger();
}
