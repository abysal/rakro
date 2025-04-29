#pragma once

#include <array>
#include <bit>
#include <chrono>
#include <cstring>
#include <expected>
#include <span>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using socklen_t = int;
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#endif

namespace rakro::detail {

#ifdef _WIN32
    using socket_t                    = SOCKET;
    using SocketError                 = int;
    constexpr socket_t invalid_socket = INVALID_SOCKET;
    inline void        close_socket(socket_t sock) { closesocket(sock); }
    inline int         get_last_error() { return WSAGetLastError(); }
#else
    using socket_t                    = int;
    constexpr socket_t invalid_socket = -1;
    inline void        close_socket(socket_t sock) { close(sock); }
    inline int         get_last_error() { return errno; }
#endif

    bool init();
    void cleanup();

    struct IPV4Addr {
        sockaddr_in address;

        bool operator==(const IPV4Addr& other) const {
            return address.sin_addr.s_addr == other.address.sin_addr.s_addr &&
                   address.sin_port == other.address.sin_port;
        }
    };

    class UdpSocket {
    public:
        explicit UdpSocket(const char* port);

        std::expected<std::pair<size_t, IPV4Addr>, SocketError>
        recv_value(std::span<uint8_t> buffer) noexcept;

        int send(std::span<uint8_t> buffer, const IPV4Addr& address);

    private:
        socket_t sock_handle{};
    };

    inline size_t time_since_epoch() noexcept {
        return static_cast<size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::system_clock::now().time_since_epoch()
        )
                                       .count());
    }

} // namespace rakro::detail

namespace std {
    template <> struct hash<rakro::detail::IPV4Addr> {
        size_t operator()(const rakro::detail::IPV4Addr& addr) const {

            std::size_t hash =
                std::hash<uint32_t>{}(std::bit_cast<uint32_t>(addr.address.sin_addr));
            hash ^= std::hash<uint16_t>{}(std::bit_cast<uint16_t>(addr.address.sin_port)) +
                    0x9e3779b9 + (hash << 6);
            return hash;
        }
    };
} // namespace std