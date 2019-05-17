#include "bundles.h"
#include "memory/executable_address_space.h"
#include "logging/log.h"
#include <fstream>

static WDepot** static_depot_pointer;

static void* vtable_WBundleDataHandleReader_000_custom[26];
static void* vtable_WBundleDataHandleReader_010_custom[3];

WBundleDiskFile* bundle_file_find(uint32_t file_index) {
  WXBundleManager& manager = *static_depot_pointer[0]->bundle_manager;

  if (file_index > 0 && file_index < manager.file_count) {
    return manager.files[file_index];
  } else {
    return nullptr;
  }
}

static WXBundleFileMapping* bundle_file_mapping(uint32_t file_index) {
  WXBundleManager& manager = *static_depot_pointer[0]->bundle_manager;
  WXBundleFileIndex& index = *manager.file_index;

  if (file_index < index.mapping_count) {
    WXBundleFileMapping* mapping = &index.mappings[file_index];

    if (mapping->file_id != 0) {
      return mapping;
    }
  }

  return nullptr;
}

WDiskBundle* bundle_file_identify(uint32_t file_index) {
  WXBundleFileMapping* mapping = bundle_file_mapping(file_index);

  if (mapping != nullptr) {
    WXBundleManager& manager = *static_depot_pointer[0]->bundle_manager;
    WXBundleFileIndex& index = *manager.file_index;

    WXBundleFileLocation& location = index.locations[mapping->file_id];

    if (location.bundle_index < manager.bundle_count) {
      return manager.bundles[location.bundle_index];
    }
  }

  return nullptr;
}

void bundle_format_file_directory(WDirectory *directory, std::wstring &path) {
  if (directory != nullptr) {
    bundle_format_file_directory(directory->parent, path);

    path.append(directory->name.text);
    path.append(L"/");
  }
}

static void custom_WBundleDataHandleReader_deconstructor(WBundleDataHandleReader* reader) {
  delete[] ((uint8_t*) reader->buffer);
  delete reader;
}

static WBundleDataHandleReader* find_replacement(const std::wstring& file_path) {
  if (file_path != L"depot/gameplay/items/def_loot_shops.xml") {
    return nullptr;
  }

  std::ifstream file(R"(Q:\Games\uncooked_witcher\def_loot_shops.xml)", std::ios::binary | std::ios::ate);
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  auto data = new uint8_t[size + sizeof(WBundleDataHandleReader)];

  if (file.read((char*) &data[sizeof(WBundleDataHandleReader)], size)) {
    auto reader = new WBundleDataHandleReader;
    reader->vtable_one = vtable_WBundleDataHandleReader_000_custom;
    reader->hardcoded_0A = 0x0A;
    reader->hardcoded_A3 = 0xA3;
    reader->vtable_two = vtable_WBundleDataHandleReader_010_custom;
    reader->p018 = nullptr;

    auto buffer = (WXBuffer<uint8_t>*) data;
    buffer->data = &data[sizeof(WBundleDataHandleReader)];
    buffer->length = size;

    reader->buffer = buffer;
    reader->read_cursor = 0;
    reader->read_limit = size;
    return reader;
  }

  return nullptr;
}

static WBundleDataHandleReader* hook_bundle_file_read(WBundleDiskFile* bundle_file, bool complex) {
  std::wstring full_path;
  bundle_format_file_directory(bundle_file->directory, full_path);
  full_path.append(bundle_file->file_name.text);

  logger::it->debug("Loading {} file {}: {}", complex, bundle_file->file_index, logger::wide(full_path));

  WXBundleFileMapping* mapping = bundle_file_mapping(bundle_file->file_index);
  WDiskBundle* bundle = bundle_file_identify(bundle_file->file_index);

  if (mapping != nullptr && bundle != nullptr) {
    logger::it->debug("... with mapping {} ->{}. Bundle {} named {}", bundle_file->file_index,
                      mapping->file_id, bundle->index,
                      logger::wide(bundle->absolute_path.text));
  }

  return nullptr;
  //return find_replacement(full_path);
}

static WBundleDataHandleReader* hook_bundle_file_read_simple(WBundleDiskFile* bundle_file) {
  return hook_bundle_file_read(bundle_file, false);
}

static WBundleDataHandleReader*  hook_bundle_file_read_complex(WBundleDiskFile* bundle_file) {
  return hook_bundle_file_read(bundle_file, true);
}

static void hook_set_bundle_file_read_simple(ExecutableAddressSpace &space, WrapperAddressSpace *wrapper_space) {
  ModifiableCode wrapper = wrapper_space->reserve(0x40);
  // push rcx
  wrapper.bytes({ 0x51 });
  // sub rsp, 20h (reserve shadow area + align)
  wrapper.bytes({ 0x48, 0x83, 0xEC, 0x20 });
  // mov rax, hook_func
  wrapper.bytes({ 0x48, 0xB8 });
  wrapper.u64((uint64_t) hook_bundle_file_read_simple);
  // call rax
  wrapper.bytes({ 0xFF, 0xD0 });
  // add rsp, 20h (remove shadow area)
  wrapper.bytes({ 0x48, 0x83, 0xC4, 0x20 });
  // pop rcx
  wrapper.bytes({ 0x59 });
  // test rax, rax
  wrapper.bytes({ 0x48, 0x85, 0xC0 });
  // jz $+1
  wrapper.bytes({ 0x74, 0x01 });
  // retn
  wrapper.bytes({ 0xC3 });
  // mov rax, [142AA43B8h] (overwritten)
  wrapper.bytes({ 0x48, 0x8B, 0x05 });
  wrapper.offset32(space.by_offset(0x2AA43B8));
  // jmp 1400929F7
  wrapper.bytes({ 0xE9 });
  wrapper.offset32(space.by_offset(0x929F7));

  ModifiableCode hook = space.modify_code(0x929F0, 0x10);
  hook.bytes({ 0xE9 });
  hook.offset32((uint64_t) wrapper.address);
  hook.bytes({ 0x90, 0x90 });
}

static void hook_set_bundle_file_read_complex(ExecutableAddressSpace &space, WrapperAddressSpace *wrapper_space) {
  ModifiableCode wrapper = wrapper_space->reserve(0x40);
  // push rcx
  wrapper.bytes({ 0x51 });
  // push rdx
  wrapper.bytes({ 0x52 });
  // push r8
  wrapper.bytes({ 0x41, 0x50 });
  // sub rsp, 20h (reserve shadow area + align)
  wrapper.bytes({ 0x48, 0x83, 0xEC, 0x20 });
  // mov rax, hook_bundle_file_read
  wrapper.bytes({ 0x48, 0xB8 });
  wrapper.u64((uint64_t) hook_bundle_file_read_complex);
  // call rax
  wrapper.bytes({ 0xFF, 0xD0 });
  // add rsp, 20h (remove shadow area)
  wrapper.bytes({ 0x48, 0x83, 0xC4, 0x20 });
  // pop r8
  wrapper.bytes({ 0x41, 0x58 });
  // pop rdx
  wrapper.bytes({ 0x5A });
  // pop rcx
  wrapper.bytes({ 0x59 });
  // test rax, rax
  wrapper.bytes({ 0x48, 0x85, 0xC0 });
  // jz $+1
  wrapper.bytes({ 0x74, 0x01 });
  // retn
  wrapper.bytes({ 0xC3 });
  // mov rax, [142AA43B8h] (overwritten)
  wrapper.bytes({ 0x48, 0x8B, 0x05 });
  wrapper.offset32(space.by_offset(0x2AA43B8));
  // jmp 1400929F7
  wrapper.bytes({ 0xE9 });
  wrapper.offset32(space.by_offset(0x92A27));

  ModifiableCode hook = space.modify_code(0x92A20, 0x10);
  hook.bytes({ 0xE9 });
  hook.offset32((uint64_t) wrapper.address);
  hook.bytes({ 0x90, 0x90 });
}

void bundles_setup(TcpServer* tcp_server, WrapperAddressSpace* wrapper_space) {
  ExecutableAddressSpace space;

  static_depot_pointer = (WDepot**) space.by_offset(0x2AA43B8);

  memcpy(vtable_WBundleDataHandleReader_000_custom, (void*) space.by_offset(0x1E13B88),
      sizeof(vtable_WBundleDataHandleReader_000_custom));
  memcpy(vtable_WBundleDataHandleReader_010_custom, (void*) space.by_offset(0x1E13C60),
      sizeof(vtable_WBundleDataHandleReader_010_custom));

  vtable_WBundleDataHandleReader_000_custom[0] = (void*) custom_WBundleDataHandleReader_deconstructor;

  hook_set_bundle_file_read_simple(space, wrapper_space);
  hook_set_bundle_file_read_complex(space, wrapper_space);

  tcp_server->add_handler(10, [] (uint16_t type, const std::vector<uint8_t>& message, const TcpMessageSender& sender) {
    std::string bundle_path("<not found>");
    std::string file_path("<not found>");

    uint32_t file_index = *(uint32_t*) &message[0];

    WXBundleManager& manager = *static_depot_pointer[0]->bundle_manager;

    if (file_index > 0 && file_index < manager.file_count) {
      WBundleDiskFile* file = manager.files[file_index];

      if (file != nullptr) {
        std::wstring full_path;
        bundle_format_file_directory(file->directory, full_path);
        full_path.append(file->file_name.text);
        file_path = logger::wide(full_path);
      }
    }

    WXBundleFileIndex& index = *manager.file_index;

    if (file_index < index.mapping_count) {
      WXBundleFileMapping& mapping = index.mappings[file_index];

      if (mapping.file_id != 0) {
        WXBundleFileLocation& location = index.locations[mapping.file_id];

        if (location.bundle_index < manager.bundle_count) {
          bundle_path = logger::wide(manager.bundles[location.bundle_index]->absolute_path.text);
        }
      }
    }

    const char* bundle_bytes = bundle_path.c_str();
    size_t bundle_bytes_length = strlen(bundle_bytes);

    const char* file_bytes = file_path.c_str();
    size_t file_bytes_length = strlen(file_bytes);

    std::vector<uint8_t> response(8 + bundle_bytes_length + file_bytes_length, 0);

    *(uint32_t*) &response[0] = bundle_bytes_length;
    memcpy(&response[4], bundle_bytes, bundle_bytes_length);
    *(uint32_t*) &response[4 + bundle_bytes_length] = file_bytes_length;
    memcpy(&response[8 + bundle_bytes_length], file_bytes, file_bytes_length);

    sender(11, response);
  });
}
