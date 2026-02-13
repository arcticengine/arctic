// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2020 Asyc
// Copyright (c) 2021 The Lasting Curator
// Copyright (c) 2021 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.


#include "engine/arctic_platform_def.h"
#if defined(ARCTIC_PLATFORM_PI)|| defined(ARCTIC_PLATFORM_MACOSX)|| defined(ARCTIC_PLATFORM_WEB)

#include "engine/arctic_platform_tcpip.h"

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#include <cerrno>
#include <cstring>
#include <string>

namespace arctic {

ConnectionSocket::ConnectionSocket(AddressFamily addressFamily,
    SocketProtocol protocol) {
  state_ = SocketState::kDisconnected;
  int family = addressFamily == AddressFamily::kIpV4 ? AF_INET : AF_INET6;
  int type = protocol == SocketProtocol::kTcp ? SOCK_STREAM : SOCK_DGRAM;
  int protocolMask =
    (protocol == SocketProtocol::kTcp ? IPPROTO_TCP : IPPROTO_UDP);
  auto result = socket(family, type, protocolMask);
  if (result == -1) {
    handle_.nix = -1;
    last_error_ = "OS failed to create socket ";
    last_error_.append(std::strerror(errno));
    return;
  }
  handle_.nix = result;
}

ConnectionSocket::~ConnectionSocket() {
  if (handle_.nix != -1) {
    close(handle_.nix);
  }
}

[[nodiscard]] SocketConnectResult ConnectionSocket::Connect(const std::string address,
    uint16_t port) {
  char port_buffer[8];
  snprintf(port_buffer, sizeof(port_buffer), "%d", port);

  sockaddr data{};
  socklen_t size = sizeof(sockaddr);
  getsockname(handle_.nix, &data, &size);

  int type;
  socklen_t length = sizeof(int);
  getsockopt(handle_.nix, SOL_SOCKET, SO_TYPE, &type, &length);

  addrinfo hints {{}, data.sa_family, type,
    type == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP};

  addrinfo* res;
  auto result = getaddrinfo(address.data(), port_buffer, &hints, &res);
  if (result != 0) {
    last_error_ = "OS failed to resolve address ";
    last_error_.append(gai_strerror(result));
    state_ = SocketState::kDisconnected;
    return SocketConnectResult::kSocketError;
  }

  result = ::connect(handle_.nix, res->ai_addr, res->ai_addrlen);
  freeaddrinfo(res);

  if (result == -1) {
    if (errno == EINPROGRESS) {
      state_ = SocketState::kConnectionInProgress;
      return SocketConnectResult::kSocketConnectionInProgress;
    }
    last_error_ = "OS failed to connect socket ";
    last_error_.append(std::strerror(errno));
    state_ = SocketState::kDisconnected;
    SocketHandle tmp;
    tmp.nix = handle_.nix;
    handle_.nix = -1;
    close(tmp.nix);
    return SocketConnectResult::kSocketError;
  }
  state_ = SocketState::kConnected;
  return SocketConnectResult::kSocketOk;
}

[[nodiscard]] SocketResult ConnectionSocket::Read(char* buffer, size_t length,
    size_t *out_size) {
  if (!out_size) {
    last_error_ = "Error: out_size argument of Read is nullptr.";
    return SocketResult::kSocketError;
  }
  auto result = recv(handle_.nix, buffer, length, 0);
  if (result == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
      *out_size = 0;
      return SocketResult::kSocketOk;
    }
    int saved_errno = errno;
    last_error_ = "OS failed to read from socket, ";
    char buff[100];
    snprintf(buff, sizeof(buff), "error code: %d, ", saved_errno);
    last_error_.append(buff);
    last_error_.append(std::strerror(saved_errno));
    SocketHandle tmp;
    tmp.nix = handle_.nix;
    handle_.nix = -1;
    close(tmp.nix);
    if (saved_errno == ECONNRESET) {
      return SocketResult::kSocketConnectionReset;
    }
    return SocketResult::kSocketError;
  }
  if (result == 0) {
    last_error_ = "Recv returned with code 0, the connection is gracefully closed.";
    SocketHandle tmp;
    tmp.nix = handle_.nix;
    handle_.nix = -1;
    close(tmp.nix);
    return SocketResult::kSocketConnectionReset;
  }

  *out_size = result;
  return SocketResult::kSocketOk;
}

[[nodiscard]] SocketResult ConnectionSocket::Write(const char* buffer,
    size_t length, size_t *out_size) {
  if (!out_size) {
    last_error_ = "Error: out_size argument of Write is nullptr.";
    return SocketResult::kSocketError;
  }
  if (handle_.nix == -1) {
    return SocketResult::kSocketError;
  }
  auto result = send(handle_.nix, buffer, length, MSG_NOSIGNAL);
  if (result == -1) {
    if (errno == EAGAIN ||
        errno == EINTR ||
        errno == EWOULDBLOCK) {
      *out_size = 0;
      return SocketResult::kSocketOk;
    }
    last_error_ = "OS failed to write to socket ";
    char buff[100];
    snprintf(buff, sizeof(buff), "error code: %d, ", errno);
    last_error_.append(buff);
    last_error_.append(std::strerror(errno));
    if (errno == ECONNRESET) {
      SocketHandle tmp;
      tmp.nix = handle_.nix;
      handle_.nix = -1;
      close(tmp.nix);
      return SocketResult::kSocketConnectionReset;
    }
    {
      SocketHandle tmp;
      tmp.nix = handle_.nix;
      handle_.nix = -1;
      close(tmp.nix);
    }
    return SocketResult::kSocketError;
  }
  *out_size = result;
  return SocketResult::kSocketOk;
}

template <typename Value>
[[nodiscard]] inline SocketResult setsockopt(SocketHandle handle, int level,
    int pName, Value value, std::string* out_last_error) {
  auto status = ::setsockopt(handle.nix, level, pName,
      reinterpret_cast<char*>(&value), sizeof(Value));
  if (status == -1) {
    *out_last_error = "OS failed to set socket option ";
    out_last_error->append(std::strerror(errno));
    return SocketResult::kSocketError;
  }
  return SocketResult::kSocketOk;
}

[[nodiscard]] SocketResult ConnectionSocket::SetTcpNoDelay(bool flag) {
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif  // SOL_TCP
  return setsockopt(handle_, SOL_TCP, TCP_NODELAY,
      static_cast<int>(flag), &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoReuseAddress(bool flag) {
  return setsockopt(handle_, SOL_SOCKET, SO_REUSEADDR,
      static_cast<int>(flag), &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoBroadcast(bool flag) {
  return setsockopt(handle_, SOL_SOCKET, SO_BROADCAST,
      static_cast<int>(flag), &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoLinger(bool flag,
    uint16_t in_linger) {
  ::linger lingerData{static_cast<u_short>(flag), in_linger};
  return setsockopt(handle_, SOL_SOCKET, SO_LINGER, lingerData, &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoReceiveTimeout(
    uint32_t timeout) {
  timeval time{static_cast<time_t>(timeout), 0};
  return setsockopt(handle_, SOL_SOCKET, SO_RCVTIMEO, time, &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoSendTimeout(
    uint32_t timeout) {
  timeval time{static_cast<time_t>(timeout), 0};
  return setsockopt(handle_, SOL_SOCKET, SO_SNDTIMEO, time, &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoSendBufferSize(int size) {
  return setsockopt(handle_, SOL_SOCKET, SO_SNDBUF, size, &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoReceiveBufferSize(int size) {
  return setsockopt(handle_, SOL_SOCKET, SO_RCVBUF, size, &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoKeepAlive(bool flag) {
  return setsockopt(handle_, SOL_SOCKET, SO_KEEPALIVE,
      static_cast<int>(flag), &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoInlineOob(bool flag) {
  return setsockopt(handle_, SOL_SOCKET, SO_OOBINLINE,
      static_cast<int>(flag), &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoNonblocking(bool flag) {
  int flags = fcntl(handle_.nix, F_GETFL, 0);
  auto result = fcntl(handle_.nix, F_SETFL,
      flag ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
  if (result == -1) {
    last_error_ = "OS failed to set O_NONBLOCK socket ";
    last_error_.append(std::strerror(errno));
    return SocketResult::kSocketError;
  }
  return SocketResult::kSocketOk;
}

bool ConnectionSocket::IsValid() const {
  return handle_.nix != -1;
}

void ConnectionSocket::UpdateConnectionInProgressState() {
  if (IsValid() && state_ == SocketState::kConnectionInProgress) {
    int socket_error = 0;
    socklen_t length = sizeof(socket_error);
    
    // Check the result of getsockopt itself
    int result = getsockopt(handle_.nix, SOL_SOCKET, SO_ERROR, &socket_error, &length);
    
    if (result < 0) {
      // getsockopt failed
      state_ = SocketState::kDisconnected;
      last_error_ = "getsockopt failed: ";
      last_error_.append(std::strerror(errno));
      return;
    }
    
    // Verify that length wasn't changed unexpectedly
    if (length != sizeof(socket_error)) {
      state_ = SocketState::kDisconnected;
      last_error_ = "getsockopt returned unexpected length";
      return;
    }
    
    
    if (socket_error == 0) {
      // SO_ERROR is 0, but let's double-check with a test write
      // Try to send 0 bytes to verify the socket is truly ready
      ssize_t test_result = ::send(handle_.nix, nullptr, 0, MSG_DONTWAIT | MSG_NOSIGNAL);
      if (test_result == 0) {
        // Socket is truly ready for writing
        state_ = SocketState::kConnected;
        return;
      } else if (test_result == -1) {
        if (errno == ENOTCONN) {
          // Socket is not actually connected yet, despite SO_ERROR being 0
          return;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
          // Socket is connected but not ready for writing yet
          state_ = SocketState::kConnected;
          return;
        } else {
          // Some other error
          state_ = SocketState::kDisconnected;
          last_error_ = "Connection test failed: ";
          last_error_.append(std::strerror(errno));
          return;
        }
      }
    }
    
    if (socket_error == EINPROGRESS || socket_error == EALREADY || socket_error == EAGAIN || socket_error == EWOULDBLOCK || socket_error == EINTR) {
      // Connection still in progress
      return;
    }
    
    // Any other error means connection failed
    state_ = SocketState::kDisconnected;
    last_error_ = "Connection failed: ";
    last_error_.append(std::strerror(socket_error));  // Use socket_error, not errno!
  }
}


ConnectionSocket::ConnectionSocket() {
  handle_.nix = -1;
  state_ = SocketState::kDisconnected;
}

ConnectionSocket& ConnectionSocket::operator=(ConnectionSocket&& rhs) noexcept {
  if (this != &rhs) {
    if (handle_.nix != -1) {
      close(handle_.nix);
    }
    handle_.nix = rhs.handle_.nix;
    rhs.handle_.nix = -1;
    state_ = rhs.state_;
    rhs.state_ = SocketState::kDisconnected;
    last_error_ = std::move(rhs.last_error_);
  }
  return *this;
}

ConnectionSocket::ConnectionSocket(ConnectionSocket&& rhs) noexcept {
  handle_.nix = rhs.handle_.nix;
  rhs.handle_.nix = -1;
  state_ = rhs.state_;
  rhs.state_ = SocketState::kDisconnected;
  last_error_ = std::move(rhs.last_error_);
}






ListenerSocket::ListenerSocket(AddressFamily addressFamily,
    SocketProtocol protocol) {
  int family = addressFamily == AddressFamily::kIpV4 ? AF_INET : AF_INET6;
  int type = protocol == SocketProtocol::kTcp ? SOCK_STREAM : SOCK_DGRAM;
  int protocolMask =
    (protocol == SocketProtocol::kTcp ? IPPROTO_TCP : IPPROTO_UDP);
  handle_.nix = socket(family, type, protocolMask);
  if (handle_.nix == -1) {
    last_error_ = "OS failed to create listener socket ";
    last_error_.append(std::strerror(errno));
  }
}


ListenerSocket::~ListenerSocket() {
  if (handle_.nix != -1) {
    close(handle_.nix);
  }
}

[[nodiscard]] SocketResult ListenerSocket::Bind(const std::string address,
    uint16_t port, size_t backlog) {
  char port_buffer[8];
  snprintf(port_buffer, sizeof(port_buffer), "%d", port);

  sockaddr data{};
  socklen_t size = sizeof(sockaddr);
  getsockname(handle_.nix, &data, &size);

  int type;
  socklen_t length = sizeof(int);
  getsockopt(handle_.nix, SOL_SOCKET, SO_TYPE, &type, &length);

  addrinfo hints {{}, data.sa_family, type,
      type == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP};

  addrinfo* res;
  auto result = getaddrinfo(address.data(), port_buffer, &hints, &res);
  if (result != 0) {
    last_error_ = "OS failed to resolve address ";
    last_error_.append(gai_strerror(result));
    return SocketResult::kSocketError;
  }

  result = ::bind(handle_.nix, res->ai_addr, res->ai_addrlen);
  freeaddrinfo(res);

  if (result != -1) {
    result = listen(handle_.nix, static_cast<int>(backlog));
  }
  if (result == -1) {
    last_error_ = "OS failed to bind listener socket ";
    last_error_.append(std::strerror(errno));
    return SocketResult::kSocketError;
  }
  return SocketResult::kSocketOk;
}

ConnectionSocket ListenerSocket::Accept() const {
  SocketHandle connection_handle;
  connection_handle.nix = ::accept(handle_.nix, nullptr, nullptr);
  return ConnectionSocket(connection_handle);
}

[[nodiscard]] SocketResult ListenerSocket::SetSoReuseAddress(bool flag) {
  int int_flag = flag;
  return setsockopt(handle_, SOL_SOCKET, SO_REUSEADDR, int_flag, &last_error_);
}

[[nodiscard]] SocketResult ListenerSocket::SetSoLinger(bool flag,
    uint16_t seconds) {
  linger linger{flag, seconds};
  return setsockopt(handle_, SOL_SOCKET, SO_LINGER, linger, &last_error_);
}

[[nodiscard]] SocketResult ListenerSocket::SetSoNonblocking(bool flag) {
  int flags = fcntl(handle_.nix, F_GETFL, 0);
  auto result = fcntl(handle_.nix, F_SETFL,
      flag ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
  if (result == -1) {
    last_error_ = "OS failed to set O_NONBLOCK socket ";
    last_error_.append(std::strerror(errno));
    return SocketResult::kSocketError;
  }
  return SocketResult::kSocketOk;
}

[[nodiscard]] SocketResult ListenerSocket::SetTcpNoDelay(bool flag) {
  #ifndef SOL_TCP
  #define SOL_TCP IPPROTO_TCP
  #endif  // SOL_TCP
    return setsockopt(handle_, SOL_TCP, TCP_NODELAY,
        static_cast<int>(flag), &last_error_);
}

bool ListenerSocket::IsValid() const {
  return handle_.nix != -1;
}

ListenerSocket::ListenerSocket() {
  handle_.nix = -1;
}

ListenerSocket& ListenerSocket::operator=(ListenerSocket&& rhs) noexcept {
  if (this != &rhs) {
    if (handle_.nix != -1) {
      close(handle_.nix);
    }
    handle_.nix = rhs.handle_.nix;
    rhs.handle_.nix = -1;
    last_error_ = std::move(rhs.last_error_);
  }
  return *this;
}

ListenerSocket::ListenerSocket(ListenerSocket&& rhs) noexcept {
  handle_.nix = rhs.handle_.nix;
  rhs.handle_.nix = -1;
  last_error_ = std::move(rhs.last_error_);
}

}  // namespace arctic

#endif  // defined(ARCTIC_PLATFORM_PI)|| defined(ARCTIC_PLATFORM_MACOSX) ||defined(ARCTIC_PLATFORM_WEB)

