#pragma once

#include "rakro/packet/magic.hpp"
#include <cstdint>
#include <stdexcept>

namespace rakro::packets {
    struct OpenConnectionRequest1 {
        uint8_t  proto_version;
        uint16_t mtu{};
    };

    constexpr static size_t udp_padding = 46;
} // namespace rakro::packets

namespace rakro {
    template <> struct BinaryDataInterface<packets::OpenConnectionRequest1> {
        static void write(packets::OpenConnectionRequest1 self, BinaryBuffer& buffer) {
            throw std::logic_error("Unimplemented");
        }

        static packets::OpenConnectionRequest1 read(BinaryBuffer& buffer) {
            (void)buffer.read_next<MagicType>();
            return {.proto_version = buffer.read_next<uint8_t>(), .mtu = [&] {
                        const auto value  = static_cast<uint16_t>(buffer.underlying().size());
                        auto       remain = buffer.remaining();

                        while (remain-- != 0) {
                            (void)buffer.read_next<uint8_t>();
                        }

                        return value;
                    }()};
        }

        static size_t size(const std::optional<packets::OpenConnectionRequest1>& /*unused*/) {
            return 100; // Random as fuck guess
        }
    };

    static_assert(BinaryData<packets::OpenConnectionRequest1>);
} // namespace rakro