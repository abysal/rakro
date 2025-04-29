#pragma once

#include "rakro/packet/frame_set.hpp"
#include "rakro/packet/packet_id.hpp"
#include <cstdint>
#include <print>
#include <rakro/internal/net.hpp>
#include <span>

namespace rakro {

    class IRakServerDebugInstrument {
    public:
        [[maybe_unused]] virtual void
        on_send(std::span<uint8_t> data, PacketId packet, detail::IPV4Addr& address) {}

        [[maybe_unused]] virtual void on_client_recv(std::span<uint8_t> data) {}

        [[maybe_unused]] virtual void
        on_invalid_frame(std::span<uint8_t> data, detail::IPV4Addr& address) {};

        [[maybe_unused]] virtual void on_out_of_order_seq_recv(
            std::span<uint8_t> data, const packets::FrameInfo& info, uint32_t expected_value
        ) {};

        [[maybe_unused]] virtual void warning_log(const std::string& warning) {};

        // Returns true to allow the function to throw an error after handling
        [[maybe_unused]] virtual bool unexpected_exist_of_data_in_oof_buffer(
            std::span<uint8_t> data, const packets::FrameInfo& info
        ) {
            return true;
        }

        [[maybe_unused]] virtual void
        unhandled_client_packet(std::span<uint8_t> data, detail::IPV4Addr& address) {}

        virtual ~IRakServerDebugInstrument() {}
    };

    namespace debugger {
        class RakServerLogDebugger : public IRakServerDebugInstrument {
        public:
            RakServerLogDebugger() = default;

            void on_client_recv(std::span<uint8_t> data) override {
                std::string output;
                std::println("Recv:");
                for (const auto byte : data) {
                    output += std::format(" 0x{:x}", byte);
                }
                std::println("{}", output);
            }

            void on_send(std::span<uint8_t> data, PacketId packet, detail::IPV4Addr& address)
                override {
                if (packet == PacketId::UnconnectedPong) {
                    return;
                }

                std::println("Sent:");

                std::string output;
                for (const auto byte : data) {
                    output += std::format(" 0x{:x}", byte);
                }
                std::println("{}", output);
            }
        };
    } // namespace debugger
} // namespace rakro