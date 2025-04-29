
#pragma once

#include "rakro/internal/binary_buffer.hpp"
#include "rakro/internal/int24_t.hpp"
#include "rakro/packet/packet_id.hpp"
#include <generator>
#include <optional>
#include <print>
#include <utility>
#include <variant>

namespace rakro::packets {
    struct Ack {
        struct MultiAck {
            uint24_t start;
            uint24_t end;
        };

        using Record = std::variant<uint24_t, MultiAck>;

        std::vector<Record> records{};

        size_t count() const noexcept { return records.size(); }

        std::generator<uint24_t> all_packets() const noexcept {
            for (const auto& record : this->records) {
                if (std::holds_alternative<MultiAck>(record)) {
                    const auto range = std::get<MultiAck>(record);
                    for (uint24_t x = range.start; x <= range.end; x++) {
                        co_yield x;
                    }
                } else {
                    const auto value = std::get<uint24_t>(record);
                    co_yield value;
                }
            }
        }
    };
} // namespace rakro::packets

namespace rakro {
    template <> struct BinaryDataInterface<packets::Ack> {

        static void write(const packets::Ack& self, BinaryBuffer& buffer) {
            buffer.write(std::to_underlying(PacketId::Ack));
            buffer.write(static_cast<uint16_t>(self.count()));

            for (size_t x = 0; x < self.count(); x++) {
                if (std::holds_alternative<packets::Ack::MultiAck>(self.records[x])) {
                    buffer.write(true);
                    const auto range = std::get<packets::Ack::MultiAck>(self.records[x]);
                    buffer.write(range.start);
                    buffer.write(range.end);
                } else {
                    buffer.write(std::get<uint24_t>(self.records[x]));
                }
            }
        }

        static packets::Ack read(BinaryBuffer& buffer) {
            auto records = std::vector<packets::Ack::Record>();

            std::println("{}", buffer.read_next_peak<uint8_t>());
            const bool multi = buffer.read_next<uint8_t>();
            if (multi) {
                const auto length = std::byteswap(buffer.read_next<uint16_t>());
                records.reserve(length);

                for (int x = 0; x < length; x++) {

                    records.push_back(packets::Ack::MultiAck{
                        .start = buffer.read_next<uint24_t>(),
                        .end   = buffer.read_next<uint24_t>()
                    });
                }
            } else {
                records.push_back(buffer.read_next<uint24_t>());
            }

            return packets::Ack{.records = records};
        }

        static size_t size(const std::optional<packets::Ack>& /*unused*/) { return 3; }
    };
    static_assert(BinaryData<packets::Ack>);
} // namespace rakro