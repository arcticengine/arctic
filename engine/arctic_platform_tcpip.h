// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2020 Asyc
// Copyright (c) 2021 The Lasting Curator
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

#ifndef ENGINE_ARCTIC_PLATFORM_TCPIP_H_
#define ENGINE_ARCTIC_PLATFORM_TCPIP_H_

#include <cstdint>

// exceptions
#include <stdexcept>
#include <string>
//

namespace arctic {

enum class AddressFamily {
  kIpV4,
  kIpV6
};

enum class SocketProtocol {
  kTcp,
  kUdp
};

enum SocketResult {
  kSocketOk = 0,
  kSocketError = 1,
  kSocketConnectionReset = 2
};

struct SocketHandle {
  union {
    uint64_t win;
    int nix;
  };
};


class ConnectionSocket {
 public:
  ConnectionSocket();
  explicit ConnectionSocket(AddressFamily family, SocketProtocol protocol);
  explicit ConnectionSocket(SocketHandle handle) {
    handle_ = handle;
  }
  ConnectionSocket(const ConnectionSocket& other) = delete;
  ConnectionSocket(ConnectionSocket&& rhs) noexcept;
  ~ConnectionSocket();

  ConnectionSocket& operator=(const ConnectionSocket& rhs) = delete;
  ConnectionSocket& operator=(ConnectionSocket&& rhs) noexcept;

  [[nodiscard]] SocketResult Connect(const std::string ip, uint16_t port);
  [[nodiscard]] SocketResult Read(char* buffer, size_t length,
      size_t *out_size);
  [[nodiscard]] SocketResult Write(const char* buffer, size_t length,
      size_t *out_size);

  template <typename Container>
  size_t Read(Container *in_out_container) const {
     return Read(in_out_container->data(), in_out_container->size());
  }

  template <typename Container>
  size_t Write(const Container& container) const {
    return Write(container.data(), container.size());
  }

  // Config Functions
  [[nodiscard]] SocketResult SetTcpNoDelay(bool flag);
  [[nodiscard]] SocketResult SetSoReuseAddress(bool flag);
  [[nodiscard]] SocketResult SetSoBroadcast(bool flag);
  [[nodiscard]] SocketResult SetSoLinger(bool flag, uint16_t seconds);
  [[nodiscard]] SocketResult SetSoReceiveTimeout(uint32_t milliseconds);
  [[nodiscard]] SocketResult SetSoSendTimeout(uint32_t milliseconds);
  [[nodiscard]] SocketResult SetSoSendBufferSize(int size);
  [[nodiscard]] SocketResult SetSoReceiveBufferSize(int size);
  [[nodiscard]] SocketResult SetSoKeepAlive(bool flag);
  [[nodiscard]] SocketResult SetSoInlineOob(bool flag);
  [[nodiscard]] SocketResult SetSoNonblocking(bool flag);
  bool IsValid() const;
  std::string GetLastError() const {
    return last_error_;
  }

 protected:
  SocketHandle handle_;
  std::string last_error_;
};

class ListenerSocket {
 public:
  ListenerSocket();
  explicit ListenerSocket(AddressFamily family, SocketProtocol protocol);
  ListenerSocket(const ListenerSocket& other) = delete;
  ListenerSocket(ListenerSocket&& rhs) noexcept;
  ~ListenerSocket();
  ListenerSocket& operator=(const ListenerSocket& rhs) = delete;
  ListenerSocket& operator=(ListenerSocket&& rhs) noexcept;
  [[nodiscard]] SocketResult Bind(const std::string address, uint16_t port,
      size_t backlog = 20);
  ConnectionSocket Accept() const;
  [[nodiscard]] SocketResult SetSoReuseAddress(bool flag);
  [[nodiscard]] SocketResult SetSoLinger(bool flag, uint16_t seconds);
  [[nodiscard]] SocketResult SetSoNonblocking(bool flag);
  bool IsValid() const;
  std::string GetLastError() const {
    return last_error_;
  }

 protected:
  SocketHandle handle_;
  std::string last_error_;
};

}  // namespace arctic

#endif  // ENGINE_ARCTIC_PLATFORM_TCPIP_H_

