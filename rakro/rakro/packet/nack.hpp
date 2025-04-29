
#pragma once

#include "rakro/internal/binary_buffer.hpp"
#include "rakro/internal/int24_t.hpp"
#include "rakro/packet/packet_id.hpp"
#include <generator>
#include <optional>
#include <utility>
#include <variant>

namespace rakro::packets {
    struct Nack {
        struct MultiNack {
            uint24_t start;
            uint24_t end;
        };

        using Record = std::variant<uint24_t, MultiNack>;

        std::vector<Record> records{};

        size_t count() const noexcept { return records.size(); }

        std::generator<uint24_t> all_packets() const noexcept {
            for (const auto& record : this->records) {
                if (std::holds_alternative<MultiNack>(record)) {
                    const auto range = std::get<MultiNack>(record);
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
    template <> struct BinaryDataInterface<packets::Nack> {

        static void write(const packets::Nack& self, BinaryBuffer& buffer) {
            buffer.write(std::to_underlying(PacketId::Nack));
            buffer.write(static_cast<uint16_t>(self.count()));

            for (size_t x = 0; x < self.count(); x++) {
                if (std::holds_alternative<packets::Nack::MultiNack>(self.records[x])) {
                    buffer.write(true);
                    const auto range = std::get<packets::Nack::MultiNack>(self.records[x]);
                    buffer.write(range.start);
                    buffer.write(range.end);
                } else {
                    buffer.write(std::get<uint24_t>(self.records[x]));
                }
            }
        }

        static packets::Nack read(BinaryBuffer& buffer) {
            const auto length  = buffer.read_next<uint16_t>();
            auto       records = std::vector<packets::Nack::Record>();
            records.reserve(length);

            for (int x = 0; x < length; x++) {
                if (buffer.read_next<bool>()) {
                    records.push_back(packets::Nack::MultiNack{
                        .start = buffer.read_next<uint24_t>(),
                        .end   = buffer.read_next<uint24_t>()
                    });
                } else {
                    records.push_back(buffer.read_next<uint24_t>());
                }
            }

            return packets::Nack{.records = records};
        }

        static size_t size(const std::optional<packets::Nack>& /*unused*/) { return 3; }
    };
    static_assert(BinaryData<packets::Nack>);
} // namespace rakro