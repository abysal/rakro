#pragma once
#include <array>
#include <rakro/internal/binary_buffer.hpp>

namespace rakro {

    constexpr std::array<uint8_t, 16> Magic = {0x00, 0xff, 0xff, 0x00, 0xfe, 0xfe, 0xfe, 0xfe,
                                               0xfd, 0xfd, 0xfd, 0xfd, 0x12, 0x34, 0x56, 0x78};
    using MagicType                         = decltype(Magic);

    template <> struct BinaryDataInterface<MagicType> {

        static void write(const MagicType& /*unused*/, BinaryBuffer& buff) {
            for (const auto byte : Magic) {
                buff.write_byte(byte);
            }
        }

        static MagicType read(BinaryBuffer& buffer) {
            for (size_t x = 0; x < Magic.size(); x++) {
                (void)buffer.next_byte();
            }
            return Magic;
        }

        static size_t size(const std::optional<MagicType>& /*unused*/) {
            return sizeof(MagicType);
        }
    };

    static_assert(BinaryData<MagicType>);

}; // namespace rakro