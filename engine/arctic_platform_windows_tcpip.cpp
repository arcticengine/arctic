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
#ifdef ARCTIC_PLATFORM_WINDOWS

#include "engine/arctic_platform_tcpip.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>

namespace arctic {

static std::string GetLastError() {
  int last_error = WSAGetLastError();
  char text[256] = {0};
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, last_error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    text, sizeof(text), NULL);
  char full_text[1024];
  snprintf(full_text, sizeof(full_text), "Error code: %d, %s", last_error, text);
  return std::string(full_text);
}

ConnectionSocket::ConnectionSocket(AddressFamily addressFamily,
    SocketProtocol protocol) {
  WSAData data{};
  auto status = WSAStartup(MAKEWORD(2, 2), &data);
  if (status != 0) {
    handle_.win = INVALID_SOCKET;
    last_error_ = "WinSock failed to initialize ";
    last_error_.append(GetLastError());
    return;
  }
  int family = addressFamily == AddressFamily::kIpV4 ? AF_INET : AF_INET6;
  int type = protocol == SocketProtocol::kTcp ? SOCK_STREAM : SOCK_DGRAM;
  int protocolMask =
    (protocol == SocketProtocol::kTcp ? IPPROTO_TCP : IPPROTO_UDP);
  handle_.win = socket(family, type, protocolMask);
}

ConnectionSocket::~ConnectionSocket() {
  if (handle_.win != INVALID_SOCKET) {
    closesocket((SOCKET)handle_.win);
    WSACleanup();
  }
}

[[nodiscard]] SocketResult ConnectionSocket::Connect(const std::string address,
    uint16_t port) {
  char port_buffer[8];
  snprintf(port_buffer, sizeof(port_buffer), "%d", port);
  WSAPROTOCOL_INFOW proto;
  WSADuplicateSocketW((SOCKET)handle_.win, GetCurrentProcessId(), &proto);
  addrinfo hints{{}, proto.iAddressFamily, proto.iSocketType, proto.iProtocol};
  addrinfo* info;
  auto result = getaddrinfo(address.data(), port_buffer, &hints, &info);
  if (result != 0) {
    last_error_ = "WinSock failed to resolve name ";
    last_error_.append(GetLastError());
    return kSocketError;
  }
  result = ::connect((SOCKET)handle_.win, info->ai_addr,
      static_cast<int>(info->ai_addrlen)) == 0;
  freeaddrinfo(info);
  if (result == 0) {
    last_error_ = "WinSock failed to bind socket ";
    last_error_.append(GetLastError());
    return kSocketError;
  }
  return kSocketOk;
}

[[nodiscard]] SocketResult ConnectionSocket::Read(char* buffer, size_t length,
    size_t *out_size) {
  if (!out_size) {
    last_error_ = "Error: out_size argument of Write is nullptr.";
    return kSocketError;
  }
  *out_size = recv((SOCKET)handle_.win, buffer, static_cast<int>(length), NULL);
  return kSocketOk;
}

[[nodiscard]] SocketResult ConnectionSocket::Write(const char* buffer,
    size_t length, size_t *out_size) {
  if (!out_size) {
    last_error_ = "Error: out_size argument of Write is nullptr.";
    return kSocketError;
  }
  auto result = send((SOCKET)handle_.win, buffer, static_cast<int>(length), NULL);
  if (result == SOCKET_ERROR) {
    last_error_ = "WinSock failed to write to socket ";
    last_error_.append(GetLastError());
    *out_size = 0;
    return kSocketError;
  }
  *out_size = length;
  return kSocketOk;
}

template <typename Value>
[[nodiscard]] inline SocketResult setsockopt(SocketHandle handle, int level,
    int opt_name, Value value, std::string *out_last_error) {
  auto status = ::setsockopt((SOCKET)handle.win, level, opt_name,
      reinterpret_cast<char*>(&value), sizeof(Value));
  if (status != 0) {
    *out_last_error = "WinSock failed to set socket option ";
    out_last_error->append(GetLastError());
    return kSocketError;
  }
  return kSocketOk;
}

[[nodiscard]] SocketResult ConnectionSocket::SetTcpNoDelay(bool flag) {
  return setsockopt(handle_, IPPROTO_TCP, TCP_NODELAY,
      static_cast<BOOL>(flag), &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoReuseAddress(bool flag) {
  return setsockopt(handle_, SOL_SOCKET, SO_REUSEADDR,
      static_cast<BOOL>(flag), &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoBroadcast(bool flag) {
  return setsockopt(handle_, SOL_SOCKET, SO_BROADCAST,
      static_cast<BOOL>(flag), &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoLinger(bool flag,
    u_short linger) {
  ::linger lingerData{static_cast<u_short>(flag), linger};
  return setsockopt(handle_, SOL_SOCKET, SO_LINGER, lingerData, &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoReceiveTimeout(
    uint32_t timeout) {
  return setsockopt(handle_, SOL_SOCKET, SO_RCVTIMEO, timeout, &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoSendTimeout(
    uint32_t timeout) {
  return setsockopt(handle_, SOL_SOCKET, SO_SNDTIMEO, timeout, &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoSendBufferSize(int size) {
  return setsockopt(handle_, SOL_SOCKET, SO_RCVBUF, size, &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoReceiveBufferSize(int size) {
  return setsockopt(handle_, SOL_SOCKET, SO_SNDBUF, size, &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoKeepAlive(bool flag) {
  return setsockopt(handle_, SOL_SOCKET, SO_KEEPALIVE,
      static_cast<BOOL>(flag), &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoInlineOob(bool flag) {
  return setsockopt(handle_, SOL_SOCKET, SO_OOBINLINE,
      static_cast<BOOL>(flag), &last_error_);
}

[[nodiscard]] SocketResult ConnectionSocket::SetSoNonblocking(bool flag) {
  u_long ulong_flag = flag ? 1U : 0U;
  auto result = ioctlsocket((SOCKET)handle_.win, FIONBIO, &ulong_flag);
  if (result != 0) {
    last_error_ = "Error: failed to set socket to nonblocking ";
    last_error_.append(GetLastError());
    return kSocketError;
  }
  return kSocketOk;
}

bool ConnectionSocket::IsValid() const {
  return handle_.win != INVALID_SOCKET;
}

ConnectionSocket::ConnectionSocket() {
  handle_.win = INVALID_SOCKET;
}

ConnectionSocket& ConnectionSocket::operator=(ConnectionSocket&& rhs) noexcept {
  if (this != &rhs) {
    SocketHandle tmp;
    tmp.win = handle_.win;
    handle_.win = rhs.handle_.win;
    rhs.handle_.win = tmp.win;
  }
  return *this;
}

ConnectionSocket::ConnectionSocket(ConnectionSocket&& rhs) noexcept {
  handle_.win = rhs.handle_.win;
  rhs.handle_.win = INVALID_SOCKET;
}


ListenerSocket::ListenerSocket(AddressFamily addressFamily,
    SocketProtocol protocol) {
  WSAData data{};
  auto status = WSAStartup(MAKEWORD(2, 2), &data);
  if (status != 0) {
    handle_.win = INVALID_SOCKET;
    last_error_ = "WinSock failed to initialize ";
    last_error_.append(GetLastError());
    return;
  }

  int family = addressFamily == AddressFamily::kIpV4 ? AF_INET : AF_INET6;
  int type = protocol == SocketProtocol::kTcp ? SOCK_STREAM : SOCK_DGRAM;
  int protocolMask =
    (protocol == SocketProtocol::kTcp ? IPPROTO_TCP : IPPROTO_UDP);
  handle_.win = socket(family, type, protocolMask);
}

ListenerSocket& ListenerSocket::operator=(ListenerSocket&& rhs) noexcept {
  if (this != &rhs) {
    SocketHandle tmp;
    tmp.win = handle_.win;
    handle_.win = rhs.handle_.win;
    rhs.handle_.win = tmp.win;
  }
  return *this;
}

ListenerSocket::~ListenerSocket() {
  if ((SOCKET)handle_.win != INVALID_SOCKET) {
    closesocket((SOCKET)handle_.win);
    WSACleanup();
  }
}

[[nodiscard]] SocketResult ListenerSocket::Bind(const std::string address,
    uint16_t port, size_t backlog) {
  char port_buffer[8];
  snprintf(port_buffer, sizeof(port_buffer), "%d", port);
  WSAPROTOCOL_INFOW proto;
  WSADuplicateSocketW((SOCKET)handle_.win, GetCurrentProcessId(), &proto);
  addrinfo hints{{}, proto.iAddressFamily, proto.iSocketType, proto.iProtocol};
  addrinfo* info;
  auto result = getaddrinfo(address.data(), port_buffer, &hints, &info);
  if (result != 0) {
    last_error_ = "WinSock failed to resolve name ";
    last_error_.append(GetLastError());
    return kSocketError;
  }
  auto status = ::bind((SOCKET)handle_.win, info->ai_addr,
      static_cast<int>(info->ai_addrlen));
  if (status != 0) {
    last_error_ = "WinSock failed to bind server socket ";
    last_error_.append(GetLastError());
    return kSocketError;
  }
  status = listen((SOCKET)handle_.win, static_cast<int>(backlog));
  if (status != 0) {
    last_error_ = "WinSock failed to listen server socket ";
    last_error_.append(GetLastError());
    return kSocketError;
  }
  return kSocketOk;
}

ConnectionSocket ListenerSocket::Accept() const {
  SocketHandle socket;
  socket.win = ::accept((SOCKET)handle_.win, nullptr, nullptr);
  return ConnectionSocket(socket);
}

[[nodiscard]] SocketResult ListenerSocket::SetSoReuseAddress(bool flag) {
  BOOL bFlag = flag;
  auto result = ::setsockopt((SOCKET)handle_.win, SOL_SOCKET, SO_REUSEADDR,
      reinterpret_cast<const char*>(&bFlag), sizeof(BOOL));
  if (result == -1) {
    last_error_ = "WinSock failed to set socket option ";
    last_error_.append(GetLastError());
    return kSocketError;
  }
  return kSocketOk;
}

[[nodiscard]] SocketResult ListenerSocket::SetSoLinger(bool flag,
    uint16_t seconds) {
  linger linger{flag, seconds};
  auto result = ::setsockopt((SOCKET)handle_.win, SOL_SOCKET, SO_LINGER,
      reinterpret_cast<char*>(&linger), sizeof(linger));
  if (result == -1) {
    last_error_ = "WinSock failed to set socket option ";
    last_error_.append(GetLastError());
    return kSocketError;
  }
  return kSocketOk;
}

[[nodiscard]] SocketResult ListenerSocket::SetSoNonblocking(bool flag) {
  u_long ulong_flag = flag ? 1U : 0U;
  auto result = ioctlsocket((SOCKET)handle_.win, FIONBIO, &ulong_flag);
  if (result != 0) {
    last_error_ = "Error: failed to set socket to nonblocking ";
    last_error_.append(GetLastError());
    return kSocketError;
  }
  return kSocketOk;
}

bool ListenerSocket::IsValid() const {
  return handle_.win != INVALID_SOCKET;
}

ListenerSocket::ListenerSocket() {
  handle_.win = INVALID_SOCKET;
}

ListenerSocket::ListenerSocket(ListenerSocket&& rhs) noexcept {
  handle_.win = rhs.handle_.win;
  rhs.handle_.win = INVALID_SOCKET;
}

}  // namespace arctic

#endif  // ARCTIC_PLATFORM_WINDOWS

