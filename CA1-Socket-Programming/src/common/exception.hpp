#ifndef SRC_COMMON_EXCEPTION_HPP
#define SRC_COMMON_EXCEPTION_HPP

#include <stdexcept>
#include <string>

class NetworkException : public std::runtime_error {
  public:
    explicit NetworkException(const std::string& message) : std::runtime_error(message) {}
};

class SocketCreationException : public NetworkException {
  public:
    explicit SocketCreationException(const std::string& message = "Failed to create socket")
        : NetworkException(message) {}
};

class BindException : public NetworkException {
  public:
    explicit BindException(const std::string& message = "Failed to bind socket")
        : NetworkException(message) {}
};

class ConnectionException : public NetworkException {
  public:
    explicit ConnectionException(const std::string& message = "Failed to establish connection")
        : NetworkException(message) {}
};

class ProtocolException : public NetworkException {
  public:
    explicit ProtocolException(const std::string& message) : NetworkException(message) {}
};

#endif // SRC_COMMON_EXCEPTION_HPP
