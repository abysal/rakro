#pragma once
#include "rak_address.hpp"
#include "rakro/packet/packet_id.hpp"
#include <utility>

namespace rakro::packets {
    struct ConnectionRequestAccepted {
        RakAddress client_address;
        /*system index is here*/
        /*system address list of 10 addresses is here*/
        uint64_t client_req_time{};
        uint64_t server_up_time{};
    };

} // namespace rakro::packets

namespace rakro {
    template <> struct BinaryDataInterface<packets::ConnectionRequestAccepted> {

        static void write(const packets::ConnectionRequestAccepted& self, BinaryBuffer& buff) {
            buff.write<uint8_t>(std::to_underlying(PacketId::ConnectionRequestAccepted));
            buff.write(self.client_address);
            buff.write<uint16_t>(0);
            for (int x = 0; x < 20; x++) {
                buff.write(packets::RakAddress{.ip = 0xFFFFFFFF, .port = 19132});
            }
            buff.write(self.client_req_time);
            buff.write(self.server_up_time);
        }

        static packets::ConnectionRequestAccepted read(BinaryBuffer& /*unused*/) {
            throw std::logic_error("unimplemented");
        }

        static size_t
        size(const std::optional<packets::ConnectionRequestAccepted>& /*unused*/) {
            return packets::RakAddress::size() + 2 + (packets::RakAddress::size() * 20) + 8 +
                   8 + 1;
        }
    };
    static_assert(BinaryData<packets::ConnectionRequestAccepted>);
} // namespace rakro