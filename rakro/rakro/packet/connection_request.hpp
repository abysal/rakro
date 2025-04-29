#pragma once

#include "rakro/internal/binary_buffer.hpp"
#include <cstdint>
#include <stdexcept>

namespace rakro::packets {
    struct ConnectionRequest {
        uint64_t client_guid;
        uint64_t request_timestamp;
        bool     secure{false};
    };

} // namespace rakro::packets

namespace rakro {
    template <> struct BinaryDataInterface<packets::ConnectionRequest> {

        static void write(const packets::ConnectionRequest& /*unused*/, BinaryBuffer& buff) {
            throw std::logic_error("unimplemented");
        }

        static packets::ConnectionRequest read(BinaryBuffer& buffer) {
            return packets::ConnectionRequest{
                .client_guid       = buffer.read_next<uint64_t>(),
                .request_timestamp = buffer.read_next<uint64_t>(),
                .secure            = buffer.read_next<bool>()
            };
        }

        static size_t size(const std::optional<packets::ConnectionRequest>& /*unused*/) {
            return 10;
        }
    };
    static_assert(BinaryData<packets::ConnectionRequest>);
} // namespace rakro