#include "server_client.hpp"
#include "rakro/internal/binary_buffer.hpp"
#include "rakro/internal/buffer_company.hpp"
#include "rakro/internal/net.hpp"
#include "rakro/packet/ack.hpp"
#include "rakro/packet/connected_ping_pong.hpp"
#include "rakro/packet/connection_request.hpp"
#include "rakro/packet/connection_request_accepted.hpp"
#include "rakro/packet/packet_id.hpp"
#include "rakro/packet/rak_address.hpp"
#include <print>
#include <rakro/internal/assert.hpp>
#include <rakro/packet/frame_set.hpp>
#include <stdexcept>

namespace rakro {

    void RakroServerClient::process_frame(BinaryBuffer packet_data, packets::FrameInfo info) {
        if (info.sequence_frame_index.has_value()) {
            if (this->debugger) {
                this->debugger->warning_log(
                    "Recived a sequenced packet, this implementation is not sound and may break"
                );
            }

            rakro_assert(info.order_info.has_value());

            const auto order_seq_index = info.order_info->order_channel;

            const auto fire_debug = [&](uint32_t expected) {
                if (this->debugger) {
                    debugger->on_out_of_order_seq_recv(
                        packet_data.underlying(), info, expected
                    );
                }
            };

            bool valid_seq = this->sequenced_packets_next_packet[order_seq_index] <=
                             info.sequence_frame_index->get_value();

            bool valid_ord = this->ordered_buffer_next_packets[order_seq_index] <=
                             info.order_info->order_frame_index.get_value();

            // If we have a packet that is invalid in some form, bail
            if (!valid_seq || !valid_ord) {
                fire_debug(this->sequenced_packets_next_packet[order_seq_index]);
                return;
            }
            this->sequenced_packets_next_packet[order_seq_index] =
                info.sequence_frame_index->get_value() + 1;
            this->process_data(std::move(packet_data));
            return;

        } else if (info.order_info.has_value()) {
            const auto order_channel = info.order_info->order_channel;
            auto key = construct_order_key(order_channel, info.order_info->order_frame_index);

            if (this->ordered_buffer_next_packets[order_channel] ==
                info.order_info->order_frame_index.get_value()) {
                // This is the packet we wanted next

                this->ordered_buffer_next_packets[order_channel]++;

                this->process_data(std::move(packet_data));

                while (this->out_of_order_packet_buffer.contains(key)) {
                    const auto val = this->out_of_order_packet_buffer.extract(key);

                    this->process_data(std::move(val.mapped().raw_data));

                    key = construct_order_key(
                        order_channel,
                        val.mapped().packet_info.order_info->order_frame_index.get_value()
                    );

                    this->ordered_buffer_next_packets[order_channel]++; // bumps to the enxt
                                                                        // packet
                }

            } else {
                // This packet arrived out-of-order

                if (this->ordered_buffer_next_packets[order_channel] >
                    info.order_info->order_frame_index.get_value()) {
                    return; // Means it arrived later than another packet
                }

                this->sequenced_packets_next_packet[order_channel] =
                    0; // Have no reason why this is here, just saw it in another implementation
                       // lol

                // Handles a case where a bug causes us to not have removed the packet from our
                // buffer
                if (this->out_of_order_packet_buffer.contains(key)) {
                    if (this->debugger) {
                        if (!this->debugger->unexpected_exist_of_data_in_oof_buffer(
                                packet_data.underlying(), info
                            ))
                            return;
                    }

                    throw std::logic_error(std::format(
                        "Out-of-order key already exists in ordering, in the {} channel, and "
                        "of index {}",
                        order_channel, info.order_info->order_frame_index.get_value()
                    ));
                }

                auto reorder_info = PacketInformation(std::move(packet_data), std::move(info));

                this->out_of_order_packet_buffer.insert({key, std::move(reorder_info)});
            }
        }

        this->process_data(std::move(packet_data));
    }

    void RakroServerClient::process_data(BinaryBuffer buffer) {
        if (this->debugger) {
            this->debugger->on_client_recv(buffer.remaining_slice());
        }

        const auto packet_id = static_cast<PacketId>(buffer.read_next<uint8_t>());

        switch (packet_id) {
        case PacketId::ConnectionRequest: {
            const auto packet_info = buffer.read_next<packets::ConnectionRequest>();

            this->guid = packet_info.client_guid;

            if (packet_info.secure) {
                return; // We cant handle security
            }

            const auto response = packets::ConnectionRequestAccepted{
                .client_address  = packets::RakAddress::from_ipv4(this->address),
                .client_req_time = packet_info.request_timestamp,
                .server_up_time  = detail::time_since_epoch() - this->server_start_time
            };

            auto send_buffer = BinaryBuffer(this->company->rent());

            send_buffer.write(std::move(response));
            this->send_to(this->ready_buffer(send_buffer, packets::FrameReliability::Reliable));
            break;
        }
        case PacketId::NewIncommingConnection: {
            break; // So basically, by this point we are already connected. and there is no
                   // information in this packet we care about, so we can just drop it and
                   // pretend it isnt a thing
        }
        case PacketId::ConnectedPingPong: {
            const auto remaining_size = buffer.remaining();
            auto       send_buffer    = BinaryBuffer(this->company->rent());
            if (remaining_size == 8) {
                const auto time = buffer.read_next<packets::ConnectedPing>();

                send_buffer.write(packets::ConnectedPong{
                    .time_since_start = time.time_since_start,
                    .time_since_server_start =
                        detail::time_since_epoch() - this->server_start_time,
                });

            } else if (remaining_size == 16) {
                const auto pong = buffer.read_next<packets::ConnectedPong>();

                send_buffer.write(
                    packets::ConnectedPing{.time_since_start = pong.time_since_start}
                );

            } else [[unlikely]] {
                // invalid packet
            }

            this->send_to(this->ready_buffer(send_buffer, packets::FrameReliability::Unreliable)
            );
        }
        default: {
            if (this->debugger) {
                this->debugger->unhandled_client_packet(
                    buffer.remaining_slice(), this->address
                );
            }
        }
        }
    }

    std::span<uint8_t> RakroServerClient::ready_buffer(
        const BinaryBuffer& buffer, packets::FrameReliability rely
    ) {

        auto send_buffer = BinaryBuffer(this->company->rent());
        send_buffer.write(this->make_header());

        send_buffer.write(packets::make_info(rely, buffer));

        for (const auto byte : buffer.consumed_slice()) {
            send_buffer.write_byte(byte);
        }

        const auto out_buff = send_buffer.consumed_slice();

        if (packets::detail::is_reliable(rely)) {
            const auto index = this->sending_rely_frame_index++;
            this->ack_buffer.insert({index, std::move(send_buffer)});
        }

        return out_buff;
    }

    void RakroServerClient::send_to(std::span<uint8_t> buffer) {
        if (this->debugger) {
            this->debugger->on_send(buffer, static_cast<PacketId>(buffer[0]), this->address);
        }

        this->socket->send(buffer, this->address);
    }

    void RakroServerClient::process_ack(BinaryBuffer buffer) {
        const auto ack = buffer.read_next<packets::Ack>();

        for (const auto id : ack.all_packets()) {
            this->ack_buffer.erase(id);
        }
    }

    void RakroServerClient::process_packet(BinaryBuffer packet_data) {

        if ((packet_data.read_next_peak<uint8_t>() & packets::VALID_FRAME_MASK) !=
            packets::VALID_FRAME_MASK) {
            return; // Usually means the client send invalid data, or we just got unlucky and a
                    // bit flipped, if so our NACK will handle it
        }

        const auto data = packet_data.read_next_peak<uint8_t>();

        if (data > packets::VALID_MAX_FRAME_ID) {
            switch (static_cast<PacketId>(data)) {
            case PacketId::Nack: {
                std::println("Unhandled NACK");
                break;
            }
            case PacketId::Ack: {
                this->process_ack(std::move(packet_data));
                break;
            }
            default:
            }
            return;
        }

        const auto frame_header = packet_data.read_next<packets::FrameHeader>();

        if (frame_header.sequence_number < this->next_expected_seq) return;
        else if (frame_header.sequence_number > this->next_expected_seq) {
            for (uint24_t current_seq = this->next_expected_seq + 1;
                 current_seq < frame_header.sequence_number; current_seq++) {
                this->missing_packets.insert(current_seq.get_value());
            }
        }

        this->next_expected_seq++;

        // 4 Is the minimum packet viable
        // 3 for its header
        // and a 1 byte payload
        while (packet_data.remaining() > 4) {
            const auto header            = packet_data.read_next<packets::FrameInfo>();
            const auto next_packet_slice = packet_data.remaining_slice();

            auto buffer =
                BinaryBuffer(RentedBuffer(next_packet_slice, nullptr), header.body_leng);

            packet_data.skipn(header.body_leng);

            if (header.fragment_info.has_value()) {
                std::println("Unhandled frame type, fragment!");
            } else {
                this->process_frame(std::move(buffer), std::move(header));
            }
        }
    }
} // namespace rakro