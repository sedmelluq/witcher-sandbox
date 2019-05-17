#pragma once

#include <stdint.h>
#include <functional>

typedef std::function<bool(uint16_t, const std::vector<uint8_t>&)> TcpMessageSender;
typedef std::function<void(uint16_t, const std::vector<uint8_t>&, const TcpMessageSender&)> TcpMessageHandler;

class TcpServer {
public:
  virtual ~TcpServer() = default;
  virtual void add_handler(uint16_t type, TcpMessageHandler handler) = 0;
  virtual void start() = 0;
};

TcpServer* tcp_server_create(int16_t port);
