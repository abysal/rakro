#pragma once
#include "magic.hpp"
#include "packet_id.hpp"
#include "rakro/internal/binary_buffer.hpp"
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

namespace rakro::packets {
    struct UnconnectedPong {
        uint64_t    time{};
        uint64_t    server_guid{};
        std::string server_id{};

        UnconnectedPong(const std::string& id, uint64_t guid)
            : server_guid(guid), server_id(id) {
            this->time =
                static_cast<size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now().time_since_epoch()
                )
                                        .count());
        }
    };
} // namespace rakro::packets

namespace rakro {
    template <> struct BinaryDataInterface<packets::UnconnectedPong> {
        static void write(packets::UnconnectedPong self, BinaryBuffer& buffer) {
            buffer.write(std::to_underlying(PacketId::UnconnectedPong));
            buffer.write(self.time);
            buffer.write(self.server_guid);
            buffer.write(Magic);
            buffer.write(self.server_id);
        }

        static packets::UnconnectedPong read(BinaryBuffer& buffer) {
            throw std::logic_error("Unhandled");
        }

        static size_t size(const std::optional<packets::UnconnectedPong>& value) {
            return sizeof(Magic) + sizeof(uint64_t) * 2 +
                   value
                       .transform([](packets::UnconnectedPong pong) -> size_t {
                           return BinaryDataInterface<std::string>::size(pong.server_id) + 1;
                       })
                       .value_or(2) +
                   sizeof(uint16_t);
        }
    };

    static_assert(BinaryData<packets::UnconnectedPong>);
} // namespace rakro