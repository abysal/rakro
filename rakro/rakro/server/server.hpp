#pragma once

#include "rakro/internal/binary_buffer.hpp"
#include "rakro/internal/net.hpp"
#include "server_client.hpp"
#include <memory>
#include <thread>
#include <unordered_map>

namespace rakro {

    class IRakServerDebugInstrument {
    public:
        virtual void on_send(std::span<uint8_t> data) {}
        virtual ~IRakServerDebugInstrument() {}
    };

    class RakServer {
    public:
        explicit RakServer(const char* port) : server_socket(detail::UdpSocket(port)) {}

        RakServer(RakServer&&) = delete;

        void update_server_id(std::string&& id) {
            this->server_id = std::make_shared<std::string>(id);
        }

        void start();

    private:
        static bool is_trivial_packet(BinaryBuffer& buffer) noexcept;

        void process_packets();

        bool handle_packet(BinaryBuffer& buffer, detail::IPV4Addr& address);

        void send_to(std::span<uint8_t> buffer, detail::IPV4Addr& address);

    private:
        detail::UdpSocket                                         server_socket;
        std::thread                                               listener_thread;
        std::atomic_bool                                          running{true};
        uint8_t                                                   protocol_version{11};
        std::atomic<std::shared_ptr<std::string>>                 server_id{};
        uint64_t                                                  server_guid{0x006ACFE3};
        std::unordered_map<detail::IPV4Addr, SemiConnectedClient> mid_connection_clients{};
        std::unique_ptr<IRakServerDebugInstrument>                instrument{nullptr};
    };
} // namespace rakro