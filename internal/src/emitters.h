#pragma once

#include "memory/wrapper_address_space.h"
#include "server/tcp_server.h"

void emitters_setup(TcpServer* tcp_server, WrapperAddressSpace* wrapper_space);
void emitters_loop();
