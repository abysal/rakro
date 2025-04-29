#pragma once

#include "rakro/internal/binary_buffer.hpp"
#include <cstdint>
#include <stdexcept>

namespace rakro::packets {
    struct ConnectedPing {
        uint64_t time_since_start{};
    };

    struct ConnectedPong {
        uint64_t time_since_start{};
        uint64_t time_since_server_start{};
    };
} // namespace rakro::packets

namespace rakro {
    template <> struct BinaryDataInterface<packets::ConnectedPing> {

        static void write(const packets::ConnectedPing& /*unused*/, BinaryBuffer& buff) {
            throw std::logic_error("unimplemented");
        }

        static packets::ConnectedPing read(BinaryBuffer& buffer) {
            return {.time_since_start = buffer.read_next<uint64_t>()};
        }

        static size_t size(const std::optional<packets::ConnectedPing>& /*unused*/) {
            return sizeof(packets::ConnectedPing);
        }
    };

    static_assert(BinaryData<packets::ConnectedPing>);

    template <> struct BinaryDataInterface<packets::ConnectedPong> {

        static void write(const packets::ConnectedPong& self, BinaryBuffer& buff) {
            buff.write(self.time_since_start);
            buff.write(self.time_since_server_start);
        }

        static packets::ConnectedPong read(BinaryBuffer& buffer) {
            throw std::logic_error("Unimplemented");
        }

        static size_t size(const std::optional<packets::ConnectedPong>& /*unused*/) {
            return sizeof(packets::ConnectedPong);
        }
    };

    static_assert(BinaryData<packets::ConnectedPong>);

} // namespace rakro