#pragma once

#include "rakro/internal/binary_buffer.hpp"
#include "rakro/internal/buffer_company.hpp"
#include "rakro/internal/net.hpp"
#include "rakro/packet/packet_id.hpp"
#include "rakro/server/debug_instrument.hpp"
#include "server_client.hpp"
#include <memory>
#include <thread>
#include <unordered_map>

namespace rakro {

    struct ServerConfig {
        size_t rented_buffer_count       = 512;
        size_t rented_buffer_size        = 2048; // Shouldnt be changed past maybe 1520
        size_t rented_block_buffer_count = 4;
    };

    class RakServer {
    public:
        explicit RakServer(const char* port, ServerConfig config)
            : server_socket(detail::UdpSocket(port)),
              renter(
                  config.rented_buffer_count, config.rented_buffer_size,
                  config.rented_block_buffer_count
              ) {}

        RakServer(RakServer&&) = delete;

        void update_server_id(std::string&& id) {
            this->server_id = std::make_shared<std::string>(id);
        }

        void start();

        void update_debugger(std::unique_ptr<IRakServerDebugInstrument> debugger) noexcept {
            this->instrument = std::move(debugger);
        }

    private:
        static bool is_trivial_packet(BinaryBuffer& buffer) noexcept;

        void process_packets();

        bool handle_packet(BinaryBuffer& buffer, detail::IPV4Addr& address);

        void
        send_to(std::span<uint8_t> buffer, detail::IPV4Addr& address, PacketId sending_packet);

    private:
        detail::UdpSocket                         server_socket;
        std::thread                               listener_thread;
        std::atomic_bool                          running{true};
        uint8_t                                   protocol_version{6}; // 11
        std::atomic<std::shared_ptr<std::string>> server_id{};
        uint64_t                                  server_guid{0xDEADC0DEAF012313};
        std::unordered_map<detail::IPV4Addr, SemiConnectedClient> mid_connection_clients{};
        std::unique_ptr<IRakServerDebugInstrument>                instrument{nullptr};
        uint64_t                                                  server_start_time{};
        BufferCompany                                             renter{};
        ClientRouter                                              router{};
    };
} // namespace rakro