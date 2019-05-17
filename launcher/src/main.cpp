#include <Windows.h>
#include <Psapi.h>
#include <atomic>
#include <string>

typedef HRESULT (*fn_DirectInput8Create) (HINSTANCE moduleInstance, DWORD version, REFIID desiredInterface,
                                          LPVOID* result, LPUNKNOWN interfaceAggregation);

typedef void (*fn_GetStartupInfoW) (STARTUPINFOW* startup_info_out);

typedef void (*fn_initialize_mod) ();

class DllLoader;

static DllLoader* global_loader;

class DllLoader {
public:
  void preload() {
    HMODULE original_library = LoadLibraryW(L"C:\\Windows\\System32\\dinput8.dll");
    original_DirectInput8Create = (fn_DirectInput8Create) GetProcAddress(original_library, "DirectInput8Create");
    original_GetStartupInfoW = (fn_GetStartupInfoW) GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetStartupInfoW");

    auto module_data = (uint8_t*) GetModuleHandleW(nullptr);
    auto headers_dos = (IMAGE_DOS_HEADER*) &module_data[0];
    auto headers_nt = (IMAGE_NT_HEADERS*) &module_data[headers_dos->e_lfanew];
    IMAGE_DATA_DIRECTORY& imports_entry = headers_nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    auto import_dlls = (IMAGE_IMPORT_DESCRIPTOR*) &module_data[imports_entry.VirtualAddress];
    size_t import_dll_count = imports_entry.Size / sizeof(*import_dlls) - 1;

    for (size_t i = 0; i < import_dll_count; i++) {
      auto dll_name = (const char*) &module_data[import_dlls[i].Name];

      if (_stricmp(dll_name, "kernel32.dll") == 0) {
        auto functions = (uintptr_t*) &module_data[import_dlls[i].FirstThunk];
        auto functions_end = (uintptr_t*) &module_data[import_dlls[i + 1].FirstThunk];
        size_t function_count = functions_end - functions;

        for (size_t j = 0; j < function_count; j++) {
          if (functions[j] == (uintptr_t) original_GetStartupInfoW) {
            DWORD previous_flags;
            VirtualProtect(&functions[j], 0x08, PAGE_READWRITE, &previous_flags);

            functions[j] = (uintptr_t) DllLoader::static_hooked_GetStartupInfoW;

            VirtualProtect(&functions[j], 0x08, previous_flags, &previous_flags);
          }
        }
      }
    }
  }

  void check_load() {
    bool expected = false;

    if (loaded.compare_exchange_strong(expected, true)) {
      load();
    }
  }

  fn_DirectInput8Create get_original_dinput8() {
    check_load();
    return original_DirectInput8Create;
  }

private:
  void load() {
    load_mods_dlls(get_mods_directory());
  }

  void load_mods_dlls(const std::wstring& mods_directory) {
    WIN32_FIND_DATAW find_item;

    std::wstring search_pattern = mods_directory + L"*";
    HANDLE find_handle = FindFirstFileW(search_pattern.c_str(), &find_item);
    bool has_entry = find_handle != INVALID_HANDLE_VALUE;

    while (has_entry) {
      if ((find_item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && _wcsnicmp(find_item.cFileName, L"mod", 3) == 0) {
        load_mod_dlls(mods_directory + find_item.cFileName);
      }

      has_entry = FindNextFileW(find_handle, &find_item);
    }

    FindClose(find_handle);
  }

  void load_mod_dlls(std::wstring mod_directory) {
    mod_directory.append(L"\\dlls\\");

    std::wstring search_pattern = mod_directory + L"*";

    WIN32_FIND_DATAW item;
    HANDLE handle = FindFirstFileW(search_pattern.c_str(), &item);
    bool has_entry = handle != INVALID_HANDLE_VALUE;

    while (has_entry) {
      if ((item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && wcslen(item.cFileName) >= 5 &&
          _wcsnicmp(&item.cFileName[wcslen(item.cFileName) - 4], L".dll", 4) == 0) {

        std::wstring dll_path = mod_directory + item.cFileName;
        HMODULE module = LoadLibraryW(dll_path.c_str());

        if (module != nullptr) {
          auto initializer = (fn_initialize_mod) GetProcAddress(module, "InitializeMod");

          if (initializer != nullptr) {
            initializer();
          }
        }
      }

      has_entry = FindNextFileW(handle, &item);
    }

    FindClose(handle);
  }

  std::wstring get_mods_directory() {
    std::wstring path(get_executable_path());

    if (path.empty()) {
      return L"";
    }

    auto position = path.find_last_of('\\');

    if (position == std::wstring::npos) {
      return L"";
    }

    path.resize(position + 1);

    path.append(L"..\\..\\Mods\\");

    wchar_t resolved_path[4096];

    if (GetFullPathNameW(path.c_str(), 4096, resolved_path, nullptr) == 0) {
      return L"";
    }

    return resolved_path;
  }

  std::wstring get_executable_path() {
    wchar_t exe_full_path[4096];

    if (GetModuleFileNameW(GetModuleHandleW(nullptr), exe_full_path, 4096)) {
      return exe_full_path;
    } else {
      return L"";
    }
  }

  void hooked_GetStartupInfoW(STARTUPINFOW* startup_info_out) {
    check_load();
    original_GetStartupInfoW(startup_info_out);
  }

  static void static_hooked_GetStartupInfoW(STARTUPINFOW* startup_info_out) {
    global_loader->hooked_GetStartupInfoW(startup_info_out);
  }

  std::atomic<bool> loaded { false };

  fn_GetStartupInfoW original_GetStartupInfoW = nullptr;
  fn_DirectInput8Create original_DirectInput8Create = nullptr;
};

extern "C" __declspec(dllexport) HRESULT DirectInput8Create(
    HINSTANCE moduleInstance, DWORD version, REFIID desiredInterface, LPVOID* result, LPUNKNOWN interfaceAggregation) {

  return global_loader->get_original_dinput8()(moduleInstance, version, desiredInterface, result, interfaceAggregation);
}

std::atomic<bool> preloaded { false };

BOOL APIENTRY DllMain(HANDLE dllInstanceHandle, DWORD callReason, LPVOID reserved) {
  if (callReason == DLL_PROCESS_ATTACH) {
    bool expected = false;

    if (preloaded.compare_exchange_strong(expected, true)) {
      global_loader = new DllLoader;
      global_loader->preload();
    }
  }

  return TRUE;
}
