#include "server.hpp"
#include "rakro/internal/binary_buffer.hpp"
#include "rakro/internal/net.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <format>
#include <print>
#include <rakro/packet/incompatible_protocol.hpp>
#include <rakro/packet/open_connection_reply_one.hpp>
#include <rakro/packet/open_connection_request_one.hpp>
#include <rakro/packet/packet_id.hpp>
#include <rakro/packet/unconnected_ping.hpp>
#include <rakro/packet/unconnected_pong.hpp>
#include <rakro/server/server.hpp>
#include <stdexcept>
#include <utility>
#include <winerror.h>

void logHexBytes(std::span<const uint8_t> data) {
    for (auto byte : data) {
        std::print("{:02x} ", byte);
    }
    std::println(""); // finish with a newline
}
namespace rakro {

    bool RakServer::is_trivial_packet(BinaryBuffer& buffer) noexcept {

        constexpr std::array<uint8_t, 3> trival_packets = {
            std::to_underlying(PacketId::UnconnectedPing1),
            std::to_underlying(PacketId::UnconnectedPing2),
            std::to_underlying(PacketId::OpenConnectionRequest1)
        };

        return std::ranges::contains(trival_packets, buffer.read_next_peak<uint8_t>());
    }

    void RakServer::start() {
        this->listener_thread = std::thread([this] { this->process_packets(); });
    }

    void RakServer::process_packets() {

        std::array<uint8_t, 2048> raw_data{}; // larger than the max MTU

        while (this->running.load(std::memory_order_relaxed)) {
            auto data_recv = this->server_socket.recv_value(raw_data);

            if (!data_recv.has_value()) {
                const auto error_code = data_recv.error();

                if (error_code == WSAEMSGSIZE) {
                    continue; // this means a fat packet was sent, and since the MTU is lower
                              // than the buffer size, it cant have came from a valid client
                }

                if (error_code == WSAETIMEDOUT) {
                    continue; // Just means no packet has hit the server in 2 seconds, just
                              // restart
                }

                throw std::runtime_error(std::format("Unknown socket error! {}", error_code));
            }

            if (data_recv.value().first < 1) {
                continue; // empty packet, might be a scanner
            }

            auto buffer = BinaryBuffer(raw_data, data_recv->first);

            if (!this->is_trivial_packet(buffer)) {
                std::println("Unhandled packet! {:x}", buffer.read_next_peak<uint8_t>());
                continue;
            }

            if (!this->handle_packet(buffer, data_recv.value().second)) {
                std::println("Unhandled packet: {:x}", buffer.underlying()[0]);
            }
        }
    }

    bool RakServer::handle_packet(BinaryBuffer& buffer, detail::IPV4Addr& address) {
        const auto id = static_cast<PacketId>(buffer.read_next<uint8_t>());

        switch (id) {
        case PacketId::UnconnectedPing1: {
            (void)buffer.read_next<packets::UnconnectedPing>();

            buffer.clear();

            buffer.write(packets::UnconnectedPong(*this->server_id.load(), this->server_guid));
            this->server_socket.send(buffer.consumed_slice(), address);
            return true;
        }
        case PacketId::OpenConnectionRequest1: {
            const auto ocr = buffer.read_next<packets::OpenConnectionRequest1>();
            logHexBytes(buffer.consumed_slice());
            buffer.clear();

            if (ocr.proto_version != this->protocol_version) {
                buffer.write(packets::IncompatibleProtocol(ocr.proto_version, this->server_guid)
                );
            } else {
                buffer.write(packets::OpenConnectionReply1(this->server_guid, ocr.mtu));
            }
            std::println("{}", ocr.mtu);
            logHexBytes(buffer.consumed_slice());

            this->server_socket.send(buffer.consumed_slice(), address);

            return true;
        }
        default: {
            return false;
        }
        }

        return true;
    }

    void RakServer::send_to(std::span<uint8_t> buffer, detail::IPV4Addr& address) {}

} // namespace rakro