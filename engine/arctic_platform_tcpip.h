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

/// @brief The result of a socket operation.
enum SocketResult {
  kSocketOk = 0, ///< The socket operation was successful.
  kSocketError = 1, ///< The socket operation failed.
  kSocketConnectionReset = 2 ///< The socket operation failed because the connection was reset. Reset means the connection was closed by the remote host.
};

struct SocketHandle {
  union {
    uint64_t win;
    int nix;
  };
};

/// @brief A socket for sending and receiving data.
class ConnectionSocket {
 public:
  ConnectionSocket();
  /// @param family The address family (IPv4 or IPv6)
  /// @param protocol The socket protocol (TCP or UDP)
  explicit ConnectionSocket(AddressFamily family, SocketProtocol protocol);

  /// @param handle The socket handle representing an existing socket
  explicit ConnectionSocket(SocketHandle handle) {
    handle_ = handle;
  }
  ConnectionSocket(const ConnectionSocket& other) = delete;
  ConnectionSocket(ConnectionSocket&& rhs) noexcept;
  ~ConnectionSocket();

  ConnectionSocket& operator=(const ConnectionSocket& rhs) = delete;
  ConnectionSocket& operator=(ConnectionSocket&& rhs) noexcept;

  /// @brief Connect to a remote host
  /// @param ip The IP address to connect to
  /// @param port The port to connect to
  /// @return The result of the connection attempt
  [[nodiscard]] SocketResult Connect(const std::string ip, uint16_t port);

  /// @brief Read data from the socket
  /// @param buffer The buffer to read into
  /// @param length The maximum number of bytes to read
  /// @param out_size Pointer to store the number of bytes actually read
  /// @return The result of the read operation
  [[nodiscard]] SocketResult Read(char* buffer, size_t length,
      size_t *out_size);

  /// @brief Write data to the socket
  /// @param buffer The buffer containing the data to write
  /// @param length The number of bytes to write
  /// @param out_size Pointer to store the number of bytes actually written
  /// @return The result of the write operation
  [[nodiscard]] SocketResult Write(const char* buffer, size_t length,
      size_t *out_size);

  /// @brief Read data into a container
  /// @param in_out_container The container to read into
  /// @return The number of bytes read
  template <typename Container>
  size_t Read(Container *in_out_container) const {
     return Read(in_out_container->data(), in_out_container->size());
  }

  /// @brief Write data from a container
  /// @param container The container containing the data to write
  /// @return The number of bytes written
  template <typename Container>
  size_t Write(const Container& container) const {
    return Write(container.data(), container.size());
  }

  // Config Functions
  /// @brief Set TCP_NODELAY option, disables Nagle's algorithm, reducing latency. 
  /// @param flag True to enable, false to disable 
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetTcpNoDelay(bool flag);

  /// @brief Set SO_REUSEADDR option, allows binding to an address that is in a TIME_WAIT state.
  /// @param flag True to enable, false to disable
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetSoReuseAddress(bool flag);

  /// @brief Set SO_BROADCAST option, allows sending packets to all hosts on the network.
  /// @param flag True to enable, false to disable
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetSoBroadcast(bool flag);

  /// @brief Set SO_LINGER option, specifies behavior on close when unsent data is present. Linger means the socket will wait for data to be sent and acknowledged before closing. This is useful to ensure all data is delivered on the network.
  /// @param flag True to enable, false to disable
  /// @param seconds The linger time in seconds, the maximum time the socket will wait for acknowledgment of data after closing.
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetSoLinger(bool flag, uint16_t seconds);

  /// @brief Set SO_RCVTIMEO option, specifies the time-out value for receiving data. This is the time the socket will wait for data to arrive.
  /// @param milliseconds The receive timeout in milliseconds, after which the socket will return an error if data has not arrived.
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetSoReceiveTimeout(uint32_t milliseconds);

  /// @brief Set SO_SNDTIMEO option, specifies the time-out value for sending data. This is the time the socket will wait for data to be sent.
  /// @param milliseconds The send timeout in milliseconds, after which the socket will return an error if data has not been sent.
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetSoSendTimeout(uint32_t milliseconds);

  /// @brief Set SO_SNDBUF option, specifies the size of the send buffer. This is the size of the buffer used to store data to be sent.
  /// @param size The send buffer size in bytes. The default is 8192 bytes.
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetSoSendBufferSize(int size);

  /// @brief Set SO_RCVBUF option, specifies the size of the receive buffer. This is the size of the buffer used to store data received from the network. 
  /// @param size The receive buffer size in bytes. The default is 8192 bytes.
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetSoReceiveBufferSize(int size);

  /// @brief Set SO_KEEPALIVE option. This option enables the automatic sending of keep-alive messages on the connection. This is useful to detect if the connection is still active.
  /// @param flag True to enable, false to disable
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetSoKeepAlive(bool flag);

  /// @brief Set SO_OOBINLINE option, specifies whether out-of-band data is received inline with normal data. This is useful to receive urgent data that needs to be processed immediately.
  /// @param flag True to enable, false to disable
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetSoInlineOob(bool flag);

  /// @brief Set socket to non-blocking mode, allows for non-blocking operations on the socket. This is useful to perform operations asynchronously.
  /// @param flag True to enable non-blocking, false for blocking
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetSoNonblocking(bool flag);

  /// @brief Check if the socket is valid. The socket is valid if it is not closed, and the handle is not zero, and the last error is empty.  
  /// @return True if the socket is valid, false otherwise
  bool IsValid() const;

  /// @brief Get the last error message. The last error message is set when an operation fails.
  /// @return The last error message as a string
  std::string GetLastError() const {
    return last_error_;
  }

 protected:
  SocketHandle handle_;
  std::string last_error_;
};

/// @brief A socket for listening for incoming connections.
class ListenerSocket {
 public:
  ListenerSocket();

  /// @param family The address family (IPv4 or IPv6)
  /// @param protocol The socket protocol (TCP or UDP)
  explicit ListenerSocket(AddressFamily family, SocketProtocol protocol);
  ListenerSocket(const ListenerSocket& other) = delete;
  ListenerSocket(ListenerSocket&& rhs) noexcept;
  ~ListenerSocket();
  ListenerSocket& operator=(const ListenerSocket& rhs) = delete;
  ListenerSocket& operator=(ListenerSocket&& rhs) noexcept;

  /// @brief Bind the socket to an address and port
  /// @param address The address to bind to
  /// @param port The port to bind to
  /// @param backlog The maximum length of the queue of pending connections. When the queue is full, new connections will be rejected.
  /// @return The result of the bind operation
  [[nodiscard]] SocketResult Bind(const std::string address, uint16_t port,
      size_t backlog = 20);

  /// @brief Accept a new connection
  /// @return A ConnectionSocket object representing the new connection
  ConnectionSocket Accept() const;

  /// @brief Set SO_REUSEADDR option. This option allows binding to an address that is in a TIME_WAIT state.
  /// @param flag True to enable, false to disable
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetSoReuseAddress(bool flag);

  /// @brief Set SO_LINGER option. This option specifies behavior on close when unsent data is present. Linger means the socket will wait for data to be sent and acknowledged before closing. This is useful to ensure all data is delivered on the network.
  /// @param flag True to enable, false to disable
  /// @param seconds The linger time in seconds. The maximum time the socket will wait for acknowledgment of data after closing.
  /// @return The result of the operation
  [[nodiscard]] SocketResult SetSoLinger(bool flag, uint16_t seconds);

  /// @brief Set socket to non-blocking mode. This option allows for non-blocking operations on the socket. This is useful to perform operations asynchronously.
  /// @param flag True to enable non-blocking, false for blocking
  [[nodiscard]] SocketResult SetSoNonblocking(bool flag);

  /// @brief Check if the socket is valid. The socket is valid if it is not closed, and the handle is not zero, and the last error is empty.  
  /// @return True if the socket is valid, false otherwise
  bool IsValid() const;

  /// @brief Get the last error message. The last error message is set when an operation fails.
  /// @return The last error message as a string
  std::string GetLastError() const {
    return last_error_;
  }

 protected:
  SocketHandle handle_;
  std::string last_error_;
};

}  // namespace arctic

#endif  // ENGINE_ARCTIC_PLATFORM_TCPIP_H_

