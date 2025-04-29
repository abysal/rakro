#pragma once

#include "rak_address.hpp"
#include "rakro/packet/magic.hpp"
#include <cstdint>
#include <stdexcept>

namespace rakro::packets {
    struct OpenConnectionRequest2 {
        uint16_t mtu{};
        uint64_t client_guid{};
    };

} // namespace rakro::packets

namespace rakro {
    template <> struct BinaryDataInterface<packets::OpenConnectionRequest2> {
        static void write(packets::OpenConnectionRequest2 self, BinaryBuffer& buffer) {
            throw std::logic_error("Unimplemented");
        }

        static packets::OpenConnectionRequest2 read(BinaryBuffer& buffer) {
            (void)buffer.read_next<MagicType>();
            (void)buffer.read_next<packets::RakAddress>();
            return packets::OpenConnectionRequest2{
                .mtu         = buffer.read_next<uint16_t>(), // MTU
                .client_guid = buffer.read_next<uint64_t>()
            };
        }

        static size_t size(const std::optional<packets::OpenConnectionRequest2>& /*unused*/) {
            return 33; // Random as fuck guess
        }
    };

    static_assert(BinaryData<packets::OpenConnectionRequest2>);
} // namespace rakro