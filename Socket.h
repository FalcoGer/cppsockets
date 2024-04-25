#pragma once

#include <arpa/inet.h>
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <netinet/in.h>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "NetAddress.h"
#include "Port.h"

class Socket
{
  public:
    enum class ESocketStatus : std::uint8_t
    {
        INIT,
        OK,
        ERROR,
        CONNECTED,
        DISCONNECTED,
        LISTENING,
        INVALID,
    };

    // NOLINTNEXTLINE(performance-enum-size)
    enum class EAddressFamily : sa_family_t
    {
        IPV6 = AF_INET6,
        IPV4 = AF_INET
    };

    // NOLINTNEXTLINE(performance-enum-size)
    enum class EProtocol : int
    {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM,
        RAW = SOCK_RAW
    };

  private:
    int           m_socketFD;
    ESocketStatus m_status;

    Port          m_port;
    NetAddress    m_address;

  protected:
    explicit Socket(int socketFD) : m_socketFD {socketFD}, m_status {ESocketStatus::INIT}
    {
        if (socketFD == -1)
        {
            throw std::runtime_error("Unable to create socket");
        }
    }

    void setStatus(ESocketStatus status) { m_status = status; }
    void setSockInfo()
    {
        sockaddr_in addr {};
        socklen_t   addrLen = sizeof(addr);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) // Using c-style APIs is horror.
        if (getsockname(m_socketFD, reinterpret_cast<sockaddr*>(&addr), &addrLen) == -1)
        {
            throw std::runtime_error("Failed to get socket address");
        }

        std::array<char, INET_ADDRSTRLEN> ipBuffer {};
        if (inet_ntop(AF_INET, &addr.sin_addr, ipBuffer.data(), ipBuffer.size()) == nullptr)
        {
            throw std::runtime_error("Failed to convert IP address to string");
        }

        m_port    = Port(ntohs(addr.sin_port));
        m_address = NetAddress {std::string(ipBuffer.begin(), ipBuffer.end())};
    }

    [[nodiscard]]
    auto getAddressFamily() const -> EAddressFamily
    {
        int       domain {};
        socklen_t optionLen = sizeof(domain);
        if (getsockopt(getFD(), SOL_SOCKET, SO_DOMAIN, &domain, &optionLen) == -1)
        {
            throw std::runtime_error("Unable to get socket domain.");
        }
        return static_cast<EAddressFamily>(domain);
    }

    [[nodiscard]]
    auto getProtcol() const -> EProtocol
    {
        int       type {};
        socklen_t optionLen = sizeof(type);
        if (getsockopt(getFD(), SOL_SOCKET, SO_TYPE, &type, &optionLen) == -1)
        {
            throw std::runtime_error("Unable to get socket type.");
        }
        return static_cast<EProtocol>(type);
    }

  public:
    Socket(const EAddressFamily ADDRESS_FAMILY, const EProtocol PROTOCOL)
            : Socket(socket(static_cast<int>(ADDRESS_FAMILY), static_cast<int>(PROTOCOL), 0))
    {}

    Socket(const Socket&)                     = delete;
    auto operator= (const Socket&) -> Socket& = delete;
    Socket(Socket&& other) noexcept
            : m_socketFD(other.m_socketFD),
              m_status {other.m_status},
              m_port {std::move(other.m_port)},
              m_address {std::move(other.m_address)}
    {
        other.m_socketFD = -1;
        other.m_status   = ESocketStatus::INVALID;
    }
    auto operator= (Socket&& other) noexcept -> Socket&
    {
        m_socketFD       = other.m_socketFD;
        m_port           = other.m_port;
        m_address        = std::move(other.m_address);
        m_status         = other.m_status;
        other.m_socketFD = -1;
        other.m_status   = ESocketStatus::INVALID;
        return *this;
    }

    void close()
    {
        if (m_socketFD != -1)
        {
            ::close(m_socketFD);
            m_socketFD = -1;
            m_status   = ESocketStatus::DISCONNECTED;
        }
        else if (m_status != ESocketStatus::DISCONNECTED)
        {
            m_status = ESocketStatus::INVALID;
        }
        else
        {
            // Leave on disconnected
        }
    }

    ~Socket() { close(); }

    [[nodiscard]]
    auto getFD() const noexcept -> int
    {
        return m_socketFD;
    }

    [[nodiscard]]
    auto getAddress() const noexcept -> NetAddress
    {
        return m_address;
    }

    [[nodiscard]]
    auto getPort() const noexcept -> Port
    {
        return m_port;
    }

    [[nodiscard]]
    auto getStatus() const noexcept -> ESocketStatus
    {
        return m_status;
    }

    [[nodiscard]]
    auto isError() const noexcept -> bool
    {
        return m_status == ESocketStatus::ERROR;
    }

    [[nodiscard]]
    auto isOpen() const noexcept -> bool
    {
        return m_status == ESocketStatus::CONNECTED || m_status == ESocketStatus::LISTENING;
    }

    [[nodiscard]]
    auto isBlocking() const -> bool
    {
        int flags = fcntl(getFD(), F_GETFL, 0);
        if (flags == -1)
        {
            throw std::runtime_error("Unable to get flags for socket.");
        }
        return (static_cast<uint32_t>(flags) & O_NONBLOCK) == 0;
    }

    auto operator== (const Socket& other) const -> bool { return m_socketFD == other.m_socketFD; }

    auto operator!= (const Socket& other) const -> bool { return m_socketFD != other.m_socketFD; }
};

inline auto operator<< (std::ostream& ostream, const Socket& socket) -> std::ostream&
{
    return (ostream << socket.getAddress().data() << ':' << socket.getPort().data());
}
