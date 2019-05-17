#include "emitters.h"
#include "memory/executable_address_space.h"
#include "engine_types.h"
#include "logging/log.h"
#include "server/message_builder.h"
#include "bundles.h"
#include <fstream>

static void* vtable_WParticleEmitter = nullptr;
static void* vtable_WDependencyLoader = nullptr;

struct TrackedParticleEmitter {
  WParticleEmitter* emitter;
  uint32_t file_index;
};

static std::mutex emitter_lock;
static std::unordered_map<WParticleEmitter*, TrackedParticleEmitter> tracked_emitters;

static void hook_emitter_parse_data(WParticleEmitter* emitter, WDependencyLoader* loader) {
  if (emitter->vtable_one != vtable_WParticleEmitter || loader->vtable_one != vtable_WDependencyLoader) {
    logger::it->debug("CParticleEmitter parse call... with GC'd instances...");
    return;
  }

  {
    std::lock_guard<std::mutex> guard(emitter_lock);
    tracked_emitters[emitter] = { emitter, loader->file->file_index };
  }

  logger::it->debug("Parsed CParticleEmitter ({:x}) data from file {}", logger::ptr(emitter), loader->file->file_index);
}

static void hook_emitter_destruct(WParticleEmitter* emitter) {
  {
    std::lock_guard<std::mutex> guard(emitter_lock);
    tracked_emitters.erase(emitter);
  }

  logger::it->debug("Destroyed CParticleEmitter ({:x})", logger::ptr(emitter));
}

struct TrackedRenderParticleEmitter {
  WRenderParticleEmitter* render_emitter;
  WParticleEmitter* emitter;
  uint32_t file_index;
};

static std::unordered_map<WRenderParticleEmitter*, TrackedRenderParticleEmitter> tracked_render_emitters;

static void hook_render_emitter_register(WRenderParticleEmitter* render_emitter, WParticleEmitter* emitter) {
  {
    std::lock_guard<std::mutex> guard(emitter_lock);

    const auto& it = tracked_emitters.find(emitter);

    if (it != tracked_emitters.end()) {
      tracked_render_emitters[render_emitter] = {
          render_emitter,
          it->second.emitter,
          it->second.file_index
      };

      logger::it->debug("Setup CRenderParticleEmitter {:x} from {:x} file {}", logger::ptr(render_emitter),
                        logger::ptr(it->second.emitter), it->second.file_index);
    } else {
      logger::it->debug("Setup CRenderParticleEmitter {:x}, but no corresponding emitter", logger::ptr(render_emitter));
    }
  }
}

static void hook_render_emitter_destruct(WRenderParticleEmitter* render_emitter) {
  {
    std::lock_guard<std::mutex> guard(emitter_lock);
    tracked_render_emitters.erase(render_emitter);
  }

  logger::it->debug("Destroyed CRenderParticleEmitter {:x}", logger::ptr(render_emitter));
}

static void hook_set_emitter_register(ExecutableAddressSpace &space, WrapperAddressSpace *wrapper_space) {
  ModifiableCode wrapper = wrapper_space->reserve(0x40);
  // push rcx
  wrapper.bytes({ 0x51 });
  // push rdx
  wrapper.bytes({ 0x52 });
  // sub rsp, 28h (reserve shadow area + align)
  wrapper.bytes({ 0x48, 0x83, 0xEC, 0x28 });
  // mov rax, hook_func
  wrapper.bytes({ 0x48, 0xB8 });
  wrapper.u64((uint64_t) hook_emitter_parse_data);
  // call rax
  wrapper.bytes({ 0xFF, 0xD0 });
  // add rsp, 28h (remove shadow area)
  wrapper.bytes({ 0x48, 0x83, 0xC4, 0x28 });
  // pop rdx
  wrapper.bytes({ 0x5A });
  // pop rcx
  wrapper.bytes({ 0x59 });
  // jmp originalfunction
  wrapper.bytes({ 0xE9 });
  wrapper.offset32(space.by_offset(0x516490));

  ModifiableCode hook = space.modify_code(0x1F3F4D0, 0x08);
  hook.u64((uint64_t) wrapper.address);
}

static void hook_set_emitter_destruct(ExecutableAddressSpace& space, WrapperAddressSpace* wrapper_space) {
  ModifiableCode wrapper = wrapper_space->reserve(0x40);
  // push rcx
  wrapper.bytes({ 0x51 });
  // sub rsp, 20h (reserve shadow area + align)
  wrapper.bytes({ 0x48, 0x83, 0xEC, 0x20 });
  // mov rax, hook_func
  wrapper.bytes({ 0x48, 0xB8 });
  wrapper.u64((uint64_t) hook_emitter_destruct);
  // call rax
  wrapper.bytes({ 0xFF, 0xD0 });
  // add rsp, 20h (remove shadow area)
  wrapper.bytes({ 0x48, 0x83, 0xC4, 0x20 });
  // pop rcx
  wrapper.bytes({ 0x59 });
  // push rbx
  wrapper.bytes({ 0x40, 0x53 });
  // sub rsp, 20h (reserve shadow area + align)
  wrapper.bytes({ 0x48, 0x83, 0xEC, 0x20 });
  // jmp originalfunction
  wrapper.bytes({ 0xE9 });
  wrapper.offset32(space.by_offset(0x518FE6));

  ModifiableCode hook = space.modify_code(0x518FE0, 0x08);
  hook.bytes({ 0xE9 });
  hook.offset32((uint64_t) wrapper.address);
  hook.bytes({ 0x90 });
}

static void hook_set_render_emitter_register(ExecutableAddressSpace &space, WrapperAddressSpace *wrapper_space) {
  ModifiableCode wrapper = wrapper_space->reserve(0x40);
  // mov rcx, r15
  wrapper.bytes({ 0x49, 0x8B, 0xCF });
  // mov rdx, r14
  wrapper.bytes({ 0x49, 0x8B, 0xD6 });
  // mov rax, hook_func
  wrapper.bytes({ 0x48, 0xB8 });
  wrapper.u64((uint64_t) hook_render_emitter_register);
  // call rax
  wrapper.bytes({ 0xFF, 0xD0 });
  // mov rax, [r15]
  wrapper.bytes({ 0x49, 0x8B, 0x07 });
  // mov rcx, r15
  wrapper.bytes({ 0x49, 0x8B, 0xCF });
  // jmp originalfunction
  wrapper.bytes({ 0xE9 });
  wrapper.offset32(space.by_offset(0xBFB04D));

  ModifiableCode hook = space.modify_code(0xBFB047, 0x08);
  hook.bytes({ 0xE9 });
  hook.offset32((uint64_t) wrapper.address);
  hook.bytes({ 0x90 });
}

static void hook_set_render_emitter_destruct(ExecutableAddressSpace &space, WrapperAddressSpace *wrapper_space) {
  ModifiableCode wrapper = wrapper_space->reserve(0x40);
  // push rcx
  wrapper.bytes({ 0x51 });
  // push rdx
  wrapper.bytes({ 0x52 });
  // sub rsp, 28h (reserve shadow area + align)
  wrapper.bytes({ 0x48, 0x83, 0xEC, 0x28 });
  // mov rax, hook_func
  wrapper.bytes({ 0x48, 0xB8 });
  wrapper.u64((uint64_t) hook_render_emitter_destruct);
  // call rax
  wrapper.bytes({ 0xFF, 0xD0 });
  // add rsp, 28h (remove shadow area)
  wrapper.bytes({ 0x48, 0x83, 0xC4, 0x28 });
  // pop rdx
  wrapper.bytes({ 0x5A });
  // pop rcx
  wrapper.bytes({ 0x59 });
  // jmp originalfunction
  wrapper.bytes({ 0xE9 });
  wrapper.offset32(space.by_offset(0xC08B20));

  ModifiableCode hook = space.modify_code(0x214B4B8, 0x08);
  hook.u64((uint64_t) wrapper.address);
}

void emitters_setup(TcpServer* tcp_server, WrapperAddressSpace* wrapper_space) {
  ExecutableAddressSpace space;

  vtable_WParticleEmitter = (void*) space.by_offset(0x1F3F478);
  vtable_WDependencyLoader = (void*) space.by_offset(0x1DDAD78);

  // Registers a WParticleEmitter after it has
  hook_set_emitter_register(space, wrapper_space);
  // Removes dead WParticleEmitters from tracking
  hook_set_emitter_destruct(space, wrapper_space);
  // Registers a WRenderParticleEmitter with data from associated WParticleEmitter
  hook_set_render_emitter_register(space, wrapper_space);
  // Removes dead WRenderParticleEmitter from tracking
  hook_set_render_emitter_destruct(space, wrapper_space);

  tcp_server->add_handler(5, [] (uint16_t type, const std::vector<uint8_t>& message, const TcpMessageSender& sender) {
    std::vector<uint8_t> response;

    {
      std::lock_guard<std::mutex> guard(emitter_lock);

      uint32_t size = tracked_render_emitters.size();
      message_append(response, size);

      for (const auto& it : tracked_render_emitters) {
        auto address = (uint64_t) it.second.render_emitter;
        std::string name = fmt::format("{:#16x}h", address);

        message_append_string(response, name);

        WBundleDiskFile* file = bundle_file_find(it.second.file_index);

        if (file != nullptr) {
          std::wstring directory;
          bundle_format_file_directory(file->directory, directory);

          message_append_string(response, directory);
          message_append_string(response, std::wstring(file->file_name.text));
        } else {
          message_append_string(response, "<unknown>");
          message_append_string(response, "<unknown>");
        }

        WDiskBundle* bundle = bundle_file_identify(it.second.file_index);

        if (bundle != nullptr) {
          message_append_string(response, std::wstring(bundle->absolute_path.text));
        } else {
          message_append_string(response, "<unknown>");
        }
      }
    }

    sender(6, response);
  });
}

typedef void (*emitter_config_parser_fn)(WMemoryFileReader* reader, WXParticleEmitterData* something);

static void overwrite_all_emitters() {
  std::vector<char> config;

  {
    std::ifstream file(R"(C:\Projects\witch\fileoverride\logx\emitter.bin)", std::ios::binary);
    config.insert(config.begin(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
  }

  ExecutableAddressSpace space;
  auto parser = (emitter_config_parser_fn) space.by_offset(0x518AE0);

  WMemoryFileReader reader {
      (void*) space.by_offset(0x1CFC3E8),
      0x100052,
      0xA3,
      (void*) space.by_offset(0x1CFC4C0),
      (uint8_t*) config.data(),
      config.size(),
      0
  };

  std::lock_guard<std::mutex> guard(emitter_lock);

  for (const auto& it : tracked_render_emitters) {
    reader.position = 0;
    parser(&reader, &it.second.render_emitter->something);
  }
}

static bool last_state = false;

void emitters_loop() {
  /*
  bool current_state = (GetKeyState(VK_INSERT) & 0x8000) != 0;

  if (current_state != last_state) {
    last_state = current_state;

    if (current_state) {
      overwrite_all_emitters();
    }
  }
   */
}
