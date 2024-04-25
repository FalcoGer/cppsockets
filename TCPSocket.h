#pragma once

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <iterator>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <utility>

#include "NetAddress.h"
#include "Socket.h"

namespace CPPSockets
{

class TCPSocket : public Socket
{
  public:
    explicit TCPSocket(int socketFD) : Socket(socketFD)
    {
        setStatus(ESocketStatus::CONNECTED);
        setSockInfo();
    }

    TCPSocket(const NetAddress& netAddress, const Port& port, const bool BLOCKING, const EAddressFamily ADDRESS_FAMILY = EAddressFamily::IPV4)
      : Socket(ADDRESS_FAMILY, EProtocol::TCP)
    {
        if (ADDRESS_FAMILY == EAddressFamily::IPV6)
        {
            sockaddr_in6 address {
              .sin6_family = static_cast<sa_family_t>(getAddressFamily()),
              .sin6_port   = htons(static_cast<std::uint16_t>(port)),
              .sin6_flowinfo {},
              .sin6_addr {},
              .sin6_scope_id {}
            };

            if (inet_pton(static_cast<sa_family_t>(getAddressFamily()), netAddress.data().c_str(), &address.sin6_addr)
                != 1)
            {
                throw std::runtime_error(std::format("Invalid address: {}", netAddress.data()));
            }
            //NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            if (connect(getFD(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1)
            {
                throw std::runtime_error(std::format("Unable to connect to remote host {}:{}", netAddress.data(), port.data()));
            }
        }
        else
        {
            sockaddr_in address {
              .sin_family = static_cast<sa_family_t>(getAddressFamily()),
              .sin_port   = htons(static_cast<std::uint16_t>(port)),
              .sin_addr {},
              .sin_zero {},
            };

            if (inet_pton(static_cast<sa_family_t>(getAddressFamily()), netAddress.data().c_str(), &address.sin_addr)
                != 1)
            {
                throw std::runtime_error(std::format("Invalid address: {}", netAddress.data()));
            }
            //NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            if (connect(getFD(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1)
            {
                throw std::runtime_error(std::format("Unable to connect to remote host {}:{}", netAddress.data(), port.data()));
            }
        }

        if (!BLOCKING)
        {
            setBlocking(false);
        }

        setSockInfo();

        setStatus(ESocketStatus::CONNECTED);
    }

    ~TCPSocket()                                    = default;
    TCPSocket(const TCPSocket&)                     = delete;
    auto operator= (const TCPSocket&) -> TCPSocket& = delete;
    TCPSocket(TCPSocket&& other) noexcept : Socket(std::move(other)) {}
    auto operator= (TCPSocket&& other) noexcept -> TCPSocket&
    {
        Socket::operator= (std::move(other));
        return *this;
    }

    auto send(const std::string& data) noexcept -> std::int64_t
    {
        const std::int64_t BYTES_SENT = ::send(getFD(), data.c_str(), data.size(), 0);

        if (BYTES_SENT == -1)
        {
            if (errno == EPIPE)
            {
                setStatus(ESocketStatus::DISCONNECTED);
            }
            else if (errno == EMSGSIZE)
            {
                static constexpr std::size_t MAX_CHUNK_SIZE = 1'024;
                std::int64_t                 totalBytesSent {};
                do
                {
                    const auto NEXT_CHUNK_SIZE =
                      std::min(MAX_CHUNK_SIZE, data.size() - static_cast<std::size_t>(totalBytesSent));
                    const auto START_OF_CHUNK = data.begin() + totalBytesSent;
                    const auto END_OF_CHUNK =
                      data.begin() + (totalBytesSent + static_cast<std::int64_t>(NEXT_CHUNK_SIZE));
                    auto               chunk            = std::string(START_OF_CHUNK, END_OF_CHUNK);
                    const std::int64_t CHUNK_BYTES_SENT = ::send(getFD(), chunk.c_str(), chunk.size(), 0);
                    if (CHUNK_BYTES_SENT == -1)
                    {
                        if (errno == EPIPE)
                        {
                            setStatus(ESocketStatus::DISCONNECTED);
                        }
                        else
                        {
                            setStatus(ESocketStatus::ERROR);
                        }
                        return CHUNK_BYTES_SENT;
                    }
                    totalBytesSent += CHUNK_BYTES_SENT;
                } while (!isError() && isOpen() && static_cast<std::size_t>(totalBytesSent) < data.size());
                return totalBytesSent;
            }
            else
            {
                setStatus(ESocketStatus::ERROR);
            }
        }
        return BYTES_SENT;
    }

    [[nodiscard]]
    auto queryConnectionClosed() noexcept -> bool
    {
        std::array<char, 1> buffer {};
        const std::int64_t  BYTES_READ = ::recv(getFD(), buffer.data(), buffer.size(), MSG_DONTWAIT | MSG_PEEK);

        [[unlikely]]
        if (BYTES_READ == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            setStatus(ESocketStatus::ERROR);
            return true;
        }

        if (BYTES_READ == 0)
        {
            setStatus(ESocketStatus::DISCONNECTED);
            return true;
        }

        return false;
    }

    [[nodiscard]]
    auto hasData() noexcept -> bool
    {
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(getFD(), &readFds);
        timeval   timeout {.tv_sec = 0, .tv_usec = 0};

        const int READY = ::select(getFD() + 1, &readFds, nullptr, nullptr, &timeout);

        [[unlikely]]
        if (READY == -1)    // actual error. when socket closed, will report ready for reading
        {
            setStatus(ESocketStatus::ERROR);
        }

        // -1 = error
        // 0 = not ready for read
        // 1 = ready for read

        return READY > 0;
    }

    [[nodiscard]]
    auto recv() noexcept -> std::optional<std::string>
    {
        std::stringstream             ss {};
        static constexpr std::size_t  BUFFER_SIZE {1024};
        std::array<char, BUFFER_SIZE> buffer {};
        std::int64_t                  bytesRead {0};
        do
        {
            bytesRead = ::recv(getFD(), buffer.data(), buffer.size(), 0);
            if (bytesRead > 0)
            {
                ss.write(buffer.data(), bytesRead);
            }
        } while (bytesRead > 0 && !isBlocking());

        if (bytesRead == 0)
        {
            setStatus(ESocketStatus::DISCONNECTED);
            return std::nullopt;
        }
        if (bytesRead == -1)
        {
            [[unlikely]]
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                setStatus(ESocketStatus::ERROR);
            }
        }
        if (ss.str().empty())
        {
            return std::nullopt;
        }
        return ss.str();
    }
};

} // namespace CPPSockets
