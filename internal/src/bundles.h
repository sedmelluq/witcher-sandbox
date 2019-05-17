#pragma once

#include "engine_types.h"
#include "server/tcp_server.h"
#include "memory/wrapper_address_space.h"
#include <cstdint>
#include <string>

WBundleDiskFile* bundle_file_find(uint32_t file_index);
WDiskBundle* bundle_file_identify(uint32_t file_index);
void bundle_format_file_directory(WDirectory *directory, std::wstring &path);

void bundles_setup(TcpServer* tcp_server, WrapperAddressSpace* wrapper_space);
