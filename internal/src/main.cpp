#include "memory/wrapper_address_space.h"
#include "server/tcp_server.h"
#include "emitters.h"
#include "logging/log.h"
#include "memory/executable_address_space.h"
#include "bundles.h"

static WrapperAddressSpace* wrapper_space;
static TcpServer* tcp_server;

static void loop_hook() {
  emitters_loop();
}

extern "C" __declspec(dllexport) void InitializeMod() {
  logger::setup_logger();
  logger::it->info("Beginning initialization");

  tcp_server = tcp_server_create(3548);
  tcp_server->start();

  ExecutableAddressSpace space;
  wrapper_space = new WrapperAddressSpace(space.create_wrapper_space());

  emitters_setup(tcp_server, wrapper_space);
  bundles_setup(tcp_server, wrapper_space);

  {
    ModifiableCode wrapper = wrapper_space->reserve(0x40);
    // mov rax, loop_hook
    wrapper.bytes({ 0x48, 0xB8 });
    wrapper.u64((uint64_t) loop_hook);
    // call rax
    wrapper.bytes({ 0xFF, 0xD0 });
    // mov rax, [rbx]
    wrapper.bytes({ 0x48, 0x8B, 0x03 });
    // mov rcx, rbx
    wrapper.bytes({ 0x48, 0x8B, 0xCB });
    // jmp 1400446B6
    wrapper.bytes({ 0xE9 });
    wrapper.offset32(space.by_offset(0x446B6));

    ModifiableCode hook = space.modify_code(0x446B0, 0x06);
    hook.bytes({ 0xE9 });
    hook.offset32((uint64_t) wrapper.address);
    hook.bytes({ 0x90 });
  }

  {
    *(uint32_t*) space.by_offset(0x2CB4C20) = 0x200;
  }
}
