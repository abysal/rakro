
#pragma once

#include "rakro/internal/binary_buffer.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
namespace rakro::packets {

    struct RakAddress {
        uint32_t ip;
        uint16_t port;

        static std::optional<RakAddress> from_bytes(std::span<uint8_t> address) {
            if (address.size() < 7) {
                return std::nullopt;
            }

            auto buff = BinaryBuffer(address);

            if (buff.next_byte() != 4) {
                return std::nullopt;
            }

            return RakAddress{
                .ip = buff.read_next<uint32_t>(), .port = buff.read_next<uint16_t>()
            };
        }

        constexpr static size_t size() noexcept { return 7; }
    };

} // namespace rakro::packets

namespace rakro {

    template <> struct BinaryDataInterface<packets::RakAddress> {

        static void write(const packets::RakAddress& self, BinaryBuffer& buff) {
            buff.write<uint8_t>(4);
            buff.write(self.ip);
            buff.write(self.port);
        }

        static packets::RakAddress read(BinaryBuffer& buffer) {
            if (buffer.read_next_peak<uint8_t>() != 4) {
                throw std::runtime_error("invalid IP");
            }

            return packets::RakAddress::from_bytes(
                       buffer.underlying().subspan(0, buffer.remaining())
            )
                .value();
        }

        static size_t size(const std::optional<packets::RakAddress>& /*unused*/) {
            return packets::RakAddress::size();
        }
    };

    static_assert(BinaryData<packets::RakAddress>);
} // namespace rakro