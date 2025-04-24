#pragma once

#include <array>
#include <bit>
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

} // namespace rakro::detail

namespace std {
    template <> struct hash<rakro::detail::IPV4Addr> {
        size_t operator()(const rakro::detail::IPV4Addr& addr) const {
            auto bytes =
                std::bit_cast<std::array<std::uint8_t, sizeof(rakro::detail::IPV4Addr)>>(addr);

            std::size_t hash = 0;
            for (auto b : bytes) {
                hash ^= std::hash<std::uint8_t>{}(b) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
    };
} // namespace std