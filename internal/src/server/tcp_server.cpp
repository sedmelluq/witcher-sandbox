#include <utility>

#include "tcp_server.h"
#include "../logging/log.h"

#include <thread>
#include <mutex>
#include <WinSock2.h>

class OnLeave {
public:
  explicit OnLeave (std::function<void()> function) : function(std::move(function)) {

  }

  ~OnLeave () {
    function();
  }

private:
  std::function<void ()> function;
};

class ActualTcpServer : public TcpServer {
public:
  explicit ActualTcpServer(uint16_t port): port(port), stop_event(nullptr) {
    std::lock_guard<std::mutex> guard(mutex);
    stop_event = CreateEventW(nullptr, true, false, nullptr);
  }

  ~ActualTcpServer() override {
    std::unique_lock<std::mutex> guard(mutex);
    SetEvent(stop_event);

    while(thread_count > 0) {
      condition.wait(guard);
    }
  }

  void add_handler(uint16_t type, TcpMessageHandler handler) override {
    logger::it->error("TCP server: registering handler for message type {}.", type);

    std::lock_guard<std::mutex> guard(mutex);
    handlers[type] = handler;
  }

  void start() override {
    std::lock_guard<std::mutex> guard(mutex);

    if (!started) {
      logger::it->info("TCP server: starting on port {}.", port);

      started = true;

      if (!try_start_thread(std::bind(&ActualTcpServer::accept_handler, this))) {
        logger::it->error("TCP server: failed to start accept thread.");
      }
    } else {
      logger::it->error("TCP server: already started, not starting.");
    }
  }

private:
  bool try_start_thread(std::function<void(void)>&& function) {
    thread_count++;

    try {
      std::thread thread(function);
      thread.detach();
      return true;
    } catch (const std::system_error& error) {
      thread_count--;
      return false;
    }
  }

  bool try_start_thread_lock(std::function<void(void)>&& function) {
    std::lock_guard<std::mutex> guard(mutex);
    return try_start_thread(std::move(function));
  }

  void end_thread() {
    std::lock_guard<std::mutex> guard(mutex);
    thread_count--;
    condition.notify_all();
  }

  void accept_handler() {
    uint32_t handle = UINT32_MAX;
    void* accept_event = nullptr;

    SetThreadDescription(GetCurrentThread(), L"TCP server accept thread");

    OnLeave exit([this, &handle, &accept_event] {
      logger::it->debug("TCP server: accept thread shutting down.");

      if (handle != UINT32_MAX) {
        closesocket(handle);
      }

      if (accept_event != nullptr) {
        WSACloseEvent(accept_event);
      }

      end_thread();
    });

    logger::it->debug("TCP server: creating listen socket.");
    handle = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);

    if (handle == UINT32_MAX) {
      WSADATA startup_data;
      WSAStartup(MAKEWORD(2,2), &startup_data);

      handle = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);

      if (handle == UINT32_MAX) {
        logger::it->debug("TCP server: failed to create listen socket.");
        return;
      }
    }

    if (!socket_configure(handle)) {
      logger::it->debug("TCP server: failed to set listen socket to non-blocking.");
      return;
    }

    sockaddr_in bind_address{};
    bind_address.sin_family = AF_INET,
    bind_address.sin_port = htons(port),
    bind_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(handle, (sockaddr*) &bind_address, sizeof(bind_address)) == SOCKET_ERROR) {
      logger::it->debug("TCP server: failed to bind listen socket.");
      return;
    }

    if (listen(handle, SOMAXCONN) == SOCKET_ERROR) {
      logger::it->debug("TCP server: failed to listen on listen socket.");
      return;
    }

    accept_event = WSACreateEvent();

    if (accept_event == WSA_INVALID_EVENT) {
      logger::it->error("TCP server: failed to create accept event object.");
      return;
    } else if (WSAEventSelect(handle, accept_event, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR) {
      logger::it->error("TCP server: failed to configure accept event object.");
      return;
    }

    void* events[2] = { stop_event, accept_event };

    logger::it->debug("TCP server: beginning accept loop.");

    while (true) {
      uint32_t result = WSAWaitForMultipleEvents(2, events, false, 30000, false);

      if (result == WSA_WAIT_TIMEOUT) {
        logger::it->debug("TCP server: accept thread still alive.");
        continue;
      } else if (result == WSA_WAIT_EVENT_0) {
        logger::it->debug("TCP server: accept thread stopping as requested.");
        return;
      } else if (result == WSA_WAIT_FAILED) {
        logger::it->error("TCP server: accept wait failed, aborting.");
        return;
      }

      WSANETWORKEVENTS fired_events;

      if (WSAEnumNetworkEvents(handle, accept_event, &fired_events) == SOCKET_ERROR) {
        logger::it->error("TCP server: accept event enumeration failed, aborting.");
        return;
      } else if ((fired_events.lNetworkEvents & FD_CLOSE) != 0) {
        logger::it->error("TCP server: accept socket closed, aborting.");
        return;
      } else if ((fired_events.lNetworkEvents & FD_ACCEPT) != 0) {
        uint32_t peer = WSAAccept(handle, nullptr, nullptr, nullptr, 0);

        if (peer != UINT32_MAX) {
          logger::it->debug("TCP server: accepting a new connection.");

          if (!try_start_thread_lock(std::bind(&ActualTcpServer::peer_handler, this, peer))) {
            logger::it->error("TCP server: failed to create peer socket thread.");
            closesocket(peer);
          }
        } else {
          logger::it->warn("TCP server: did not find anything to accept.");
        }
      }
    }
  }

  void peer_handler(uint32_t peer) {
    void* io_event = nullptr;

    SetThreadDescription(GetCurrentThread(), L"TCP server peer thread");

    OnLeave exit([this, peer, &io_event] {
      logger::it->debug("TCP server: peer thread shutting down.");

      if (io_event != nullptr) {
        WSACloseEvent(io_event);
      }

      closesocket(peer);
      end_thread();
    });

    if (!socket_configure(peer)) {
      logger::it->debug("TCP server: failed to set peer socket to non-blocking.");
      return;
    }

    io_event = WSACreateEvent();

    WSAEventSelect(peer, io_event, FD_READ | FD_WRITE | FD_CLOSE);

    /*auto reader = [this, peer, read_event] (std::vector<uint8_t>& buffer, size_t required_size) -> bool {
      return network_io("read", peer, read_event, FD_READ, required_size, [peer, &buffer] (size_t offset, size_t length) -> int32_t {
        return recv(peer, (char*) &buffer[offset], length, 0);
      });
    };

    auto writer = [this, peer, write_event] (const std::vector<uint8_t>& buffer, size_t required_size) -> bool {
      return network_io("write", peer, write_event, FD_WRITE, required_size, [peer, &buffer] (size_t offset, size_t length) -> int32_t {
        return send(peer, (const char*) &buffer[offset], length, 0);
      });
    };*/

    std::vector<uint8_t> header;
    header.resize(6);

    bool success = true;

    TcpMessageSender sender = [this, peer, io_event, &header, &success] (uint16_t type, const std::vector<uint8_t>& message) -> bool {
      logger::it->debug("TCP server: sending response message: type {}, length {}.", type, message.size());

      *(uint16_t*) &header[0] = type;
      *(uint32_t*) &header[2] = message.size();

      success = peer_write(peer, io_event, header) && peer_write(peer, io_event, message);
      return success;
    };

    std::vector<uint8_t> unknown_response;
    std::vector<uint8_t> message_body;

    while (peer_read(peer, io_event, header)) {
      uint16_t message_type = *(uint16_t*) &header[0];
      uint32_t message_length = *(uint32_t*) &header[2];

      logger::it->debug("TCP server: received message header: type {}, length {}.", message_type, message_length);

      if (message_length > 0x100000) {
        logger::it->error("TCP server: message length too high, aborting connection.");
        return;
      }

      message_body.resize(message_length);

      if (!peer_read(peer, io_event, message_body)) {
        logger::it->debug("TCP server: message body not read, closing peer.");
        return;
      }

      TcpMessageHandler* handler;

      auto it = handlers.find(message_type);

      if (it == handlers.end()) {
        unknown_response.resize(2);
        *(uint16_t*) &unknown_response[0] = message_type;

        sender(1, unknown_response);
      } else {
        it->second(message_type, message_body, sender);
      }

      if (!success) {
        logger::it->debug("TCP server: message sending not performed, closing peer.");
        return;
      }
    }

    logger::it->debug("TCP server: message header not read, closing peer.");
  }

  bool socket_configure(uint32_t handle) {
    u_long non_blocking = 1;
    return ioctlsocket(handle, FIONBIO, &non_blocking) != SOCKET_ERROR;
  }

  bool network_wait(uint32_t peer, void* io_event) {
    void* events[] = { stop_event, io_event };
    uint32_t result = WSAWaitForMultipleEvents(2, events, false, WSA_INFINITE, false);

    if (result == WSA_WAIT_EVENT_0) {
      logger::it->debug("TCP server: on network wait, detected stop instruction.");
      return false;
    } else if (result == WSA_WAIT_FAILED) {
      logger::it->error("TCP server: on network wait, received wait failure.");
      return false;
    }

    WSANETWORKEVENTS fired_events;

    if (WSAEnumNetworkEvents(peer, io_event, &fired_events) == SOCKET_ERROR) {
      logger::it->error("TCP server: on network wait, failed to enumerate events.");
      return false;
    } else if ((fired_events.lNetworkEvents & FD_CLOSE) != 0) {
      logger::it->warn("TCP server: on network wait, detected socket close.");
      return false;
    }

    return true;
  }

  bool peer_read(uint32_t peer, void* io_event, std::vector<uint8_t>& buffer) {
    size_t total_received = 0;

    while (total_received < buffer.size()) {
      int32_t received = recv(peer, (char*) &buffer[total_received], buffer.size() - total_received, 0);

      if (received == 0) {
        logger::it->debug("TCP server: on peer read, detected close via recv.");
        return false;
      } else if (received == SOCKET_ERROR) {
        int32_t last_error = WSAGetLastError();

        if (last_error == WSAEWOULDBLOCK) {
          if (!network_wait(peer, io_event)) {
            return false;
          }
        }
      } else {
        total_received += received;
      }
    }

    return true;
  }

  bool peer_write(uint32_t peer, void* io_event, const std::vector<uint8_t>& buffer) {
    size_t total_sent = 0;

    while (total_sent < buffer.size()) {
      int32_t sent = send(peer, (const char*) &buffer[total_sent], buffer.size() - total_sent, 0);

      if (sent == SOCKET_ERROR) {
        int32_t last_error = WSAGetLastError();

        if (last_error == WSAEWOULDBLOCK) {
          if (!network_wait(peer, io_event)) {
            return false;
          }
        }
      } else {
        total_sent += sent;
      }
    }

    return true;
  }

  /*bool network_io(const char* name, uint32_t peer, void* io_event, uint32_t io_flag, size_t required,
                  const std::function<int32_t(uint32_t, uint32_t)>& operation) {

    WSANETWORKEVENTS fired_events;
    void* events[] = { stop_event, io_event };

    size_t total_moved = 0;

    while (true) {
      logger::it->debug("TCP server: on peer {}, started wait.", name);
      uint32_t result = WSAWaitForMultipleEvents(2, events, false, WSA_INFINITE, false);
      logger::it->debug("TCP server: on peer {}, finished wait.", name);

      if (result == WSA_WAIT_EVENT_0) {
        logger::it->debug("TCP server: on peer {}, detected stop instruction.", name);
        return false;
      } else if (result == WSA_WAIT_FAILED) {
        logger::it->error("TCP server: on peer {}, received wait failure.", name);
        return false;
      } else if (WSAEnumNetworkEvents(peer, io_event, &fired_events) == SOCKET_ERROR) {
        logger::it->error("TCP server: on peer {}, failed to enumerate events.", name);
        return false;
      } else if ((fired_events.lNetworkEvents & FD_CLOSE) != 0) {
        logger::it->warn("TCP server: on peer {}, detected socket close.", name);
        return false;
      } else if ((fired_events.lNetworkEvents & io_flag) != 0) {
        int32_t current_moved = operation(total_moved, required - total_moved);

        logger::it->debug("TCP server: on peer {}, moved {}.", name, current_moved);

        if (current_moved == SOCKET_ERROR) {
          int32_t error = WSAGetLastError();

          if (error != WSAEWOULDBLOCK) {
            logger::it->warn("TCP server: on peer {}, unexpected error code {}.", name, error);
            return false;
          } else {
            continue;
          }
        } else {
          total_moved += current_moved;

          if (total_moved == required) {
            return true;
          }
        }
      }
    }
  }*/

  std::mutex mutex;
  std::condition_variable condition;
  bool started = false;
  uint32_t thread_count = 0;
  void* stop_event;
  std::unordered_map<uint16_t, TcpMessageHandler> handlers;
  int port;
};

TcpServer* tcp_server_create(int16_t port) {
  return new ActualTcpServer(port);
}
