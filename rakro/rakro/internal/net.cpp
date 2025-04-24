#include "net.hpp"
#include <expected>
#include <print>
#include <rakro/internal/net.hpp>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

namespace rakro::detail {
    bool init() {
#ifdef _WIN32
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
        return true;
#endif
    }

    void cleanup() {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    int UdpSocket::send(std::span<uint8_t> buffer, const IPV4Addr& address) {
        return sendto(
            this->sock_handle, reinterpret_cast<const char*>(buffer.data()),
            static_cast<int>(buffer.size()), 0,
            reinterpret_cast<const sockaddr*>(&address.address), sizeof(address.address)
        );
    }

    std::expected<std::pair<size_t, IPV4Addr>, SocketError>
    UdpSocket::recv_value(std::span<uint8_t> buffer) noexcept {

        sockaddr_storage storage;
        int              size = sizeof(storage);

        const auto value = recvfrom(
            this->sock_handle, reinterpret_cast<char*>(buffer.data()),
            static_cast<int>(buffer.size()), 0, reinterpret_cast<sockaddr*>(&storage), &size
        );

        if (value == SOCKET_ERROR) {
            return std::unexpected(get_last_error());
        } else {

            if (sizeof(sockaddr_in) != size) {
                return recv_value(buffer);
            }

            return std::make_pair(
                static_cast<size_t>(value),
                IPV4Addr{.address = *reinterpret_cast<sockaddr_in*>(&storage)}
            );
        }
    }

    UdpSocket::UdpSocket(const char* port) {

        static bool init_already = false;

        if (!init_already) {
            init();
            init_already = true;
        }

        int result;

        addrinfo hints = {};

        hints.ai_flags    = AI_PASSIVE;
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        addrinfo* addr_info;

        const auto* port_hint = port;

        result = getaddrinfo(nullptr, port_hint, &hints, &addr_info);

        if (result != 0) {
            throw std::runtime_error(
                std::format("Failed to get address info: {} error code", result)
            );
        }

        const auto sock =
            socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);

        const auto base_addr =
            std::unique_ptr<addrinfo, decltype(&FreeAddrInfo)>(addr_info, &FreeAddrInfo);

        if (sock == INVALID_SOCKET) {
            throw std::runtime_error(
                std::format("Failed to open socket: {} error", get_last_error())
            );
        }

        result = bind(sock, base_addr->ai_addr, static_cast<int>(base_addr->ai_addrlen));

        if (result != 0) {
            throw std::runtime_error(
                std::format("Failed to bind socket: {} error", get_last_error())
            );
        }

        int timeout = 2000; // 2 seconds

        result = setsockopt(
            sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout),
            sizeof(timeout)
        );

        if (result != 0) {
            throw std::runtime_error(
                std::format("Failed to set socket: {} error", get_last_error())
            );
        }

        this->sock_handle = sock;
    }
} // namespace rakro::detail