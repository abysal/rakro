#pragma once
#include "magic.hpp"
#include "rakro/internal/binary_buffer.hpp"
#include <cstdint>
#include <rakro/packet/magic.hpp>

namespace rakro::packets {
    struct UnconnectedPing {
        uint64_t time{0};
        uint64_t client_guid{0};
    };

} // namespace rakro::packets

namespace rakro {
    template <> struct BinaryDataInterface<packets::UnconnectedPing> {
        static void write(packets::UnconnectedPing self, BinaryBuffer& buffer) {
            buffer.write(self.time);
            buffer.write(Magic);
            buffer.write(self.client_guid);
        }

        static packets::UnconnectedPing read(BinaryBuffer& buffer) {
            return {.time = buffer.read_next<uint64_t>(), .client_guid = [&]() {
                        (void)buffer.read_next<MagicType>();
                        return buffer.read_next<uint64_t>();
                    }()};
        }

        static size_t size(const std::optional<packets::UnconnectedPing>& /*unused*/) {
            return sizeof(MagicType) + sizeof(packets::UnconnectedPing);
        }
    };

    static_assert(BinaryData<packets::UnconnectedPing>);
} // namespace rakro