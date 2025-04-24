#pragma once

#include "packet_id.hpp"
#include "rakro/packet/magic.hpp"
#include <cstdint>
#include <rakro/packet/open_connection_request_one.hpp>
#include <stdexcept>
#include <utility>

namespace rakro::packets {
    struct OpenConnectionReply1 {
        uint64_t server_guid{0xDEADC0DE};
        uint16_t MTU{};

        // Have to add + 1 for use security false and Magic is + 16
    };

} // namespace rakro::packets

namespace rakro {
    template <> struct BinaryDataInterface<packets::OpenConnectionReply1> {
        static void write(packets::OpenConnectionReply1 self, BinaryBuffer& buffer) {
            buffer.write(std::to_underlying(PacketId::OpenConnectionReply1));
            buffer.write(Magic);
            buffer.write(self.server_guid);
            buffer.write(false);
            buffer.write(self.MTU);

            // for (int x = 0; x < 32; x++) {
            //     buffer.write_byte(0xCD);
            // }
        }

        static packets::OpenConnectionReply1 read(BinaryBuffer& buffer) {
            throw std::logic_error("unimplemented");
        }

        static size_t size(const std::optional<packets::OpenConnectionReply1>& /*unused*/) {
            return sizeof(packets::OpenConnectionReply1) + 2 + sizeof(MagicType);
        }
    };

    static_assert(BinaryData<packets::OpenConnectionReply1>);
} // namespace rakro