#pragma once

#include "packet_id.hpp"
#include "rak_address.hpp"
#include "rakro/packet/magic.hpp"
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace rakro::packets {
    struct OpenConnectionReply2 {
        uint64_t   server_guid{0xDEADC0DE};
        uint16_t   MTU{};
        RakAddress client_address{};
    };

} // namespace rakro::packets

namespace rakro {
    template <> struct BinaryDataInterface<packets::OpenConnectionReply2> {
        static void write(packets::OpenConnectionReply2 self, BinaryBuffer& buffer) {
            buffer.write(std::to_underlying(PacketId::OpenConnectionReply2));
            buffer.write(Magic);
            buffer.write(self.server_guid);
            buffer.write(self.client_address);
            buffer.write(self.MTU);
            buffer.write(false);
        }

        static packets::OpenConnectionReply2 read(BinaryBuffer& buffer) {
            throw std::logic_error("unimplemented");
        }

        static size_t size(const std::optional<packets::OpenConnectionReply2>& /*unused*/) {
            return sizeof(uint64_t) + sizeof(uint16_t) + packets::RakAddress::size() +
                   sizeof(MagicType) + sizeof(uint8_t) + sizeof(bool);
        }
    };

    static_assert(BinaryData<packets::OpenConnectionReply2>);
} // namespace rakro