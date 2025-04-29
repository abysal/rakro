#pragma once

#include "rakro/internal/binary_buffer.hpp"
#include "rakro/internal/buffer_company.hpp"
#include "rakro/internal/net.hpp"
#include "rakro/packet/frame_set.hpp"
#include "rakro/server/debug_instrument.hpp"
#include <cstdint>
#include <rakro/internal/strong_typed_int_hash.hpp>
#include <unordered_map>
#include <unordered_set>

namespace rakro {

    using OrderKey                      = BaseStrongType<uint32_t>;
    constexpr size_t MAX_ORDER_CHANNELS = 32;

    inline OrderKey construct_order_key(uint8_t order_channel, uint24_t order_index) noexcept {
        uint32_t base = static_cast<uint32_t>(order_channel)
                        << 27; // Clips the order channel to 5 bits
        base |= order_index.get_value();

        return OrderKey(base);
    }

    struct SemiConnectedClient {
        uint64_t connection_time{};
        uint16_t mtu{};
    };

    // TODO: Make this run on multiple threads using a threadpool interface

    class RakroServerClient {
    public:
        struct PacketInformation {
            BinaryBuffer       raw_data{};
            packets::FrameInfo packet_info{};
            PacketInformation(BinaryBuffer&& buffer, packets::FrameInfo&& info)
                : raw_data(std::forward<BinaryBuffer>(buffer)),
                  packet_info(std::forward<packets::FrameInfo>(info)) {}
            PacketInformation()                    = default;
            PacketInformation(PacketInformation&&) = default;
        };

    public:
        RakroServerClient() = default;
        RakroServerClient(
            uint64_t guid, IRakServerDebugInstrument* debugger, detail::UdpSocket* socket,
            uint16_t mtu, BufferCompany* company, detail::IPV4Addr address,
            uint64_t server_start_time
        )
            : guid(guid), debugger(debugger), socket(socket), company(company), mtu(mtu),
              address(address), server_start_time(server_start_time) {}
        RakroServerClient(RakroServerClient&&)      = default;
        RakroServerClient(const RakroServerClient&) = delete;

        void process_packet(BinaryBuffer buffer);

        uint64_t get_guid() const noexcept { return this->guid; }

    private:
        void process_frame(BinaryBuffer buffer, packets::FrameInfo info);
        void process_data(BinaryBuffer buffer);

        void send_to(std::span<uint8_t> buffer);

        void process_ack(BinaryBuffer buffer);

        packets::FrameHeader make_header() const noexcept {
            return packets::FrameHeader{.sequence_number = this->next_expected_seq};
        }

        std::span<uint8_t>
        ready_buffer(const BinaryBuffer& buffer, packets::FrameReliability rely);

    private:
        uint24_t                   next_expected_seq{0};
        uint24_t                   sending_rely_frame_index{0};
        uint64_t                   guid{};
        IRakServerDebugInstrument* debugger{nullptr};
        detail::UdpSocket*         socket{nullptr};
        BufferCompany*             company{nullptr};
        uint16_t                   mtu{0};
        detail::IPV4Addr           address{};
        uint64_t                   last_packet{detail::time_since_epoch()};
        uint64_t                   server_start_time{};

        std::unordered_set<uint24_t>               missing_packets{};
        std::unordered_map<uint24_t, BinaryBuffer> ack_buffer{};

        // List of the next expected order number
        std::array<uint32_t, MAX_ORDER_CHANNELS>        ordered_buffer_next_packets{};
        std::unordered_map<OrderKey, PacketInformation> out_of_order_packet_buffer{};
        // Stores any packets which arrived earlier than expected

        // Stores the next expected sequence number
        std::array<uint32_t, MAX_ORDER_CHANNELS> sequenced_packets_next_packet{};

        friend class ClientRouter;
    };

    class ClientRouter {
    public:
        ClientRouter()                    = default;
        ClientRouter(const ClientRouter&) = delete;

        bool is_connected(detail::IPV4Addr addr) const noexcept {
            return this->connected_clients.contains(addr);
        }

        void route(detail::IPV4Addr address, BinaryBuffer&& buffer) noexcept {
            if (this->is_connected(address)) {
                auto& client = this->connected_clients[address];

                const auto current_time = detail::time_since_epoch();

                if (current_time - client.last_packet > this->timeout) {
                    this->connected_clients.erase(address);
                    return;
                }

                client.last_packet = current_time;

                client.process_packet(std::move(buffer));
            }
        }

        void connect_client(
            detail::UdpSocket* socket, detail::IPV4Addr address, SemiConnectedClient client,
            uint64_t guid, IRakServerDebugInstrument* debugger, BufferCompany* company,
            uint64_t server_start_time
        ) noexcept {
            if (this->is_connected(address)) {
                return;
            }

            // const auto current_time = detail::time_since_epoch();

            // for (const auto& [addr, our_client] : this->connected_clients) {
            //     if (current_time - our_client.last_packet > this->timeout) {
            //         this->connected_clients.erase(addr);
            //     }
            // }

            this->connected_clients.insert(
                {address,
                 RakroServerClient(
                     guid, debugger, socket, client.mtu, company, address, server_start_time
                 )}
            );
        }

    private:
        std::unordered_map<detail::IPV4Addr, RakroServerClient> connected_clients{};
        uint64_t                                                timeout{5000};
    };

} // namespace rakro