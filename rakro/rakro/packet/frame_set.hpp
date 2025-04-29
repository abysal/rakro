#pragma once

#include "rakro/internal/binary_buffer.hpp"
#include "rakro/internal/int24_t.hpp"
#include <optional>
#include <utility>

namespace rakro::packets {
    constexpr size_t VALID_FRAME_MASK   = 0x80;
    constexpr size_t VALID_MAX_FRAME_ID = 0x8D;
    constexpr size_t IS_ORDERD          = 0x10;

    struct FrameHeader {
        uint24_t sequence_number{};
    };

    // These are sort of strange,
    //
    // Ordered packets force any out-of-order transmissions to wait until the previous packets
    // have arrived before being processed. An example is:
    //
    // Packets are sent in an order of: 1 - 2 - 3 - 4 - 5
    //
    // Packets arrive in an order of: 1 - 2 - 5 - 3 - 4
    //
    // The processing order would be: process 1 and 2 since they are fine.
    // Get 5 — since it's not the next number we expected (we expected 3), store it.
    // Get 3 — now process 3 immediately.
    // Get 4 — now process 4 immediately.
    // Since 5 was stored and is now the next expected, process 5 too.
    //
    //
    // Sequenced packets say that they should be delivered in order,
    // but if an earlier packet arrives after a newer one has already been processed, just
    // discard it. An example:
    //
    // Packets are sent in an order of: 1 - 2 - 3 - 4 - 5
    //
    // Packets arrive in an order of: 1 - 2 - 3 - 5 - 4
    //
    // Processing order:
    // - Process 1, 2, 3 normally.
    // - Receive 5 and process it immediately (it's newer).
    // - Receive 4 and discard it (because we've already processed 5, and 4 is now considered
    // too old).
    //
    //
    // Reliable implies that we send an ACK when we successfully receive a packet,
    // and send a NACK if we detect a missing packet
    //
    // Some extra information is, it seems sequenced packets are also always ordered?
    enum class FrameReliability : uint8_t {
        Unreliable,
        UnreliableSequenced,
        Reliable,
        ReliableOrdered,
        ReliableSequenced,
        UnreliableWithAckReceipt,
        ReliableWithAckReceipt,
        ReliableOrderedWithAckReceipt
    };

    namespace detail {
        inline bool is_reliable(FrameReliability rely) {
            return (rely == FrameReliability::Reliable) ||
                   (rely == FrameReliability::ReliableOrdered) ||
                   (rely == FrameReliability::ReliableSequenced);
        }

        inline bool is_seq(FrameReliability rely) {
            return (rely == FrameReliability::UnreliableSequenced) ||
                   (rely == FrameReliability::ReliableSequenced);
        }

        inline bool is_ordered(FrameReliability rely) {
            return (rely == FrameReliability::UnreliableSequenced) ||
                   (rely == FrameReliability::ReliableOrdered) ||
                   (rely == FrameReliability::ReliableSequenced) ||
                   (rely == FrameReliability::ReliableOrderedWithAckReceipt);
        }
    } // namespace detail

    struct FrameInfo {
        constexpr static uint8_t packet_id = 0x80;

        uint16_t                body_leng{};
        FrameReliability        rely{};
        std::optional<uint24_t> reliability_index{};
        std::optional<uint24_t> sequence_frame_index{};

        struct OrderInformation {
            uint24_t order_frame_index{};
            uint8_t  order_channel{0};
        };

        std::optional<OrderInformation> order_info{};

        struct FragmentInformation {
            uint32_t fragment_size{};
            uint16_t fragment_compound_id{};
            uint32_t fragment_index{};
        };

        std::optional<FragmentInformation> fragment_info{};
    };

} // namespace rakro::packets

namespace rakro {
    template <> struct BinaryDataInterface<packets::FrameHeader> {
        static void write(packets::FrameHeader self, BinaryBuffer& buffer) {
            buffer.write(packets::FrameInfo::packet_id);
            buffer.write(self.sequence_number);
        }

        static packets::FrameHeader read(BinaryBuffer& buffer) {
            (void)buffer.read_next<uint8_t>();
            return {.sequence_number = buffer.read_next<uint24_t>()};
        }

        static size_t size(const std::optional<packets::FrameHeader>& val) {
            return 4 - static_cast<size_t>(val.has_value());
        }
    };

    static_assert(BinaryData<packets::FrameHeader>);
} // namespace rakro

namespace rakro {
    template <> struct BinaryDataInterface<packets::FrameInfo> {
        static void write(packets::FrameInfo self, BinaryBuffer& buffer) {
            buffer.write<uint8_t>(
                std::to_underlying(self.rely) | ((self.fragment_info.has_value()) ? 0x4 : 0)
            );
            buffer.write<uint16_t>(static_cast<uint16_t>(self.body_leng << 3));

            if (self.reliability_index.has_value()) {
                buffer.write<uint24_t>(self.reliability_index.value());
            }

            if (self.sequence_frame_index.has_value()) {
                buffer.write<uint24_t>(self.sequence_frame_index.value());
            }

            if (self.order_info.has_value()) {
                buffer.write<uint24_t>(self.order_info->order_frame_index);
                buffer.write<uint8_t>(self.order_info->order_channel);
            }

            if (self.fragment_info.has_value()) {
                buffer.write(self.fragment_info->fragment_size);
                buffer.write(self.fragment_info->fragment_compound_id);
                buffer.write(self.fragment_info->fragment_index);
            }
        }

        static packets::FrameInfo read(BinaryBuffer& buffer) {
            const auto flags = buffer.next_byte();

            const auto         rely = static_cast<packets::FrameReliability>(flags >> 5);
            packets::FrameInfo info{};

            info.body_leng = static_cast<uint16_t>(buffer.read_next<uint16_t>() >> 3);

            if (packets::detail::is_reliable(rely)) {
                info.reliability_index = buffer.read_next<uint24_t>();
            }

            if (packets::detail::is_seq(rely)) {
                info.sequence_frame_index = buffer.read_next<uint24_t>();
            }

            if (packets::detail::is_ordered(rely)) {
                info.order_info = packets::FrameInfo::OrderInformation{
                    .order_frame_index = buffer.read_next<uint24_t>(),
                    .order_channel     = buffer.next_byte()
                };
            }

            if (flags & packets::IS_ORDERD) {
                info.fragment_info = packets::FrameInfo::FragmentInformation{
                    .fragment_size        = buffer.read_next<uint32_t>(),
                    .fragment_compound_id = buffer.read_next<uint16_t>(),
                    .fragment_index       = buffer.read_next<uint32_t>()
                };
            }

            return info;
        }

        static size_t size(const std::optional<packets::FrameInfo>& val) {
            if (val.has_value()) {
                const auto& value = val.value();
                return 3 + value.reliability_index.has_value() * 3 +
                       value.sequence_frame_index.has_value() * 3 +
                       value.order_info.has_value() * 4 + value.fragment_info.has_value() * 10;
            } else {
                return 3; // Smallest Possible header
            }
        }
    };

    static_assert(BinaryData<packets::FrameInfo>);

    namespace packets {
        [[maybe_unused]]
        static FrameInfo make_info(
            FrameReliability rely, const BinaryBuffer& data,
            std::optional<uint24_t>                       reliability_index    = std::nullopt,
            std::optional<uint24_t>                       sequence_frame_index = std::nullopt,
            std::optional<FrameInfo::OrderInformation>    order_info           = std::nullopt,
            std::optional<FrameInfo::FragmentInformation> fragment_info        = std::nullopt
        ) {
            const auto info = FrameInfo{
                .body_leng            = static_cast<uint16_t>(data.consumed()),
                .rely                 = rely,
                .reliability_index    = reliability_index,
                .sequence_frame_index = sequence_frame_index,
                .order_info           = order_info,
                .fragment_info        = fragment_info
            };
            return info;
        }
    } // namespace packets
} // namespace rakro