#pragma once
#include "packet_id.hpp"
#include "rakro/packet/magic.hpp"
#include <cstdint>
#include <stdexcept>

namespace rakro::packets {
    struct IncompatibleProtocol {
        uint8_t  client_protocol{};
        uint64_t server_guid{};
    };
} // namespace rakro::packets

namespace rakro {
    template <> struct BinaryDataInterface<packets::IncompatibleProtocol> {
        static void write(packets::IncompatibleProtocol self, BinaryBuffer& buffer) {
            buffer.write(std::to_underlying(PacketId::IncompatibleProtocol)); // still 0x19
            buffer.write(self.client_protocol);                               // 1 byte
            buffer.write(Magic);                                              // 16 bytes
            buffer.write(self.server_guid);
        }

        static packets::IncompatibleProtocol read(BinaryBuffer& buffer) {
            throw std::logic_error("unimplemented");
        }

        static size_t size(const std::optional<packets::IncompatibleProtocol>& /*unused*/) {
            return 1 + 1 + sizeof(MagicType) + sizeof(uint64_t); // ID + Protocol byte + Magic
        }
    };

    static_assert(BinaryData<packets::IncompatibleProtocol>);
} // namespace rakro
