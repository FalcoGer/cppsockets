#pragma once

#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <functional>
#include <netinet/in.h>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <sys/socket.h>

#include "NetAddress.h"
#include "Port.h"
#include "Socket.h"
#include "TCPSocket.h"

namespace CPPSockets
{

class ListeningSocket : public Socket
{
  public:
    ListeningSocket(const NetAddress& bindAddr, const Port& port, const bool BLOCKING, const EAddressFamily ADDRESS_FAMILY = EAddressFamily::IPV4)
            : Socket(ADDRESS_FAMILY, EProtocol::TCP)
    {
        std::uint32_t enableReuse {1};
        if (setsockopt(getFD(), SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(enableReuse)) == -1)
        {
            throw std::runtime_error("Failed to set socket option SO_REUSEADDR");
        }

        if (setsockopt(getFD(), SOL_SOCKET, SO_REUSEPORT, &enableReuse, sizeof(enableReuse)) == -1)
        {
            throw std::runtime_error("Failed to set socket option SO_REUSEPORT");
        }

        if (ADDRESS_FAMILY == EAddressFamily::IPV6)
        {
            sockaddr_in6 address {
              .sin6_family = static_cast<sa_family_t>(getAddressFamily()),
              .sin6_port   = htons(static_cast<std::uint16_t>(port)),
              .sin6_flowinfo {},
              .sin6_addr {},
              .sin6_scope_id {}
            };

            if (inet_pton(static_cast<sa_family_t>(getAddressFamily()), bindAddr.data().c_str(), &address.sin6_addr)
                != 1)
            {
                throw std::runtime_error(std::format("Invalid bind address: {}", bindAddr.data()));
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) // Marginally better than C.
            if (bind(getFD(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1)
            {
                throw std::runtime_error("Failed to bind socket");
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

            if (inet_pton(static_cast<sa_family_t>(getAddressFamily()), bindAddr.data().c_str(), &address.sin_addr)
                != 1)
            {
                throw std::runtime_error(std::format("Invalid bind address: {}", bindAddr.data()));
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) // Marginally better than C.
            if (bind(getFD(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1)
            {
                throw std::runtime_error("Failed to bind socket");
            }
        }

        if (listen(getFD(), SOMAXCONN) == -1)
        {
            throw std::runtime_error("Failed to listen on socket");
        }

        if (!BLOCKING)
        {
            int flags = fcntl(getFD(), F_GETFL, 0);
            if (flags == -1)
            {
                throw std::runtime_error("fcntl get flags failed");
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) // Is there an alternative?
            if (fcntl(getFD(), F_SETFL, static_cast<std::uint32_t>(flags) | O_NONBLOCK) == -1)
            {
                throw std::runtime_error("fcntl set flags failed");
            }
        }

        setSockInfo();

        setStatus(ESocketStatus::LISTENING);
    }

    ListeningSocket(const ListeningSocket&)                     = delete;
    auto operator= (const ListeningSocket&) -> ListeningSocket& = delete;

    ListeningSocket(ListeningSocket&& other) noexcept : Socket(std::move(other)) {}
    auto operator= (ListeningSocket&& other) noexcept -> ListeningSocket&
    {
        Socket::operator= (std::move(other));
        return *this;
    }

    ~ListeningSocket() = default;

    [[nodiscard]]
    auto accept() const -> std::optional<TCPSocket>
    {
        // accept is a blocking operation if the socket is blocking
        sockaddr_in clientAddr {};
        socklen_t   clientLen = sizeof(clientAddr);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) // Marginally better than C.
        const int   FD        = ::accept(getFD(), reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);

        if (FD == -1)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                return std::nullopt;
            }
        }
        return TCPSocket(FD);
    }
};

} // namespace CPPSockets
