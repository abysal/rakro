
#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>

namespace rakro {
    class BinaryBuffer;

    template <typename T> struct BinaryDataInterface {
        static void   write(T&&, BinaryBuffer&)          = delete;
        static T      read(BinaryBuffer&)                = delete;
        static size_t size(const std::optional<T>& data) = delete;
    };

    template <typename T>
    concept BinaryData = requires(BinaryBuffer& buffer) {
        { BinaryDataInterface<T>::write(std::declval<T>(), buffer) } -> std::same_as<void>;
        { BinaryDataInterface<T>::read(buffer) } -> std::same_as<T>;
        { BinaryDataInterface<T>::size(std::declval<T>()) } -> std::same_as<size_t>;
    };

    class BinaryBuffer {
    public:
        explicit BinaryBuffer(
            std::span<uint8_t> buffer, size_t artificial_limit = static_cast<size_t>(-1)
        ) {

            this->real_buffer = buffer;

            if (artificial_limit == static_cast<size_t>(-1)) {
                this->bytes = this->real_buffer;
            } else {
                this->bytes = this->real_buffer.subspan(0, artificial_limit);
            }
        };

        template <BinaryData T> T read_next() {
            this->bounds_check(BinaryDataInterface<T>::size(std::nullopt));

            return BinaryDataInterface<T>::read(*this);
        }

        template <BinaryData T> T read_next_peak() {
            this->bounds_check(BinaryDataInterface<T>::size(std::nullopt));

            const auto current_idx = this->index;

            const auto return_val = BinaryDataInterface<T>::read(*this);
            this->index           = current_idx;
            return return_val;
        }

        template <BinaryData T> void write(T&& val) {
            this->bounds_check(BinaryDataInterface<T>::size(val));

            BinaryDataInterface<T>::write(std::forward<T>(val), *this);
        }

        template <BinaryData T> void write(const T& val) {
            this->bounds_check(BinaryDataInterface<T>::size(val));

            BinaryDataInterface<T>::write(std::forward<T>(val), *this);
        }

        template <BinaryData T> void write(T& val) {
            this->bounds_check(BinaryDataInterface<T>::size(val));

            BinaryDataInterface<T>::write(std::forward<T>(val), *this);
        }

        uint8_t next_byte() {
            this->bounds_check(1);
            return this->bytes[this->index++];
        }

        void write_byte(uint8_t val) {
            this->bounds_check(1);
            this->bytes[this->index++] = val;
        }

        uint8_t* raw() const { return this->bytes.data(); }

        size_t consumed() const { return this->index; }

        size_t clear(size_t limit = static_cast<size_t>(-1)) {
            auto access_index = this->index;
            this->index       = 0;
            if (limit == static_cast<size_t>(-1)) {
                this->bytes = this->real_buffer;
            } else {
                this->bytes = this->real_buffer.subspan(0, limit);
            }

            return access_index;
        }

        std::span<uint8_t> underlying() { return this->bytes; }

        std::span<uint8_t> consumed_slice() { return this->bytes.subspan(0, this->consumed()); }

        size_t remaining() const noexcept { return this->size() - this->index; }

        size_t size() const noexcept { return this->bytes.size(); }

    private:
        [[maybe_unused]] bool bounds_check(size_t extra) {
            if (this->index + extra - 1 >= bytes.size()) {
                throw std::out_of_range("Not enough space in type");
            }
            return true;
        }

    private:
        std::span<uint8_t> bytes{};
        std::span<uint8_t> real_buffer{};
        size_t             index{0};
    };
} // namespace rakro

#define INTEGRAL_BINARY(type)                                                                  \
    namespace rakro {                                                                          \
        template <> struct BinaryDataInterface<type> {                                         \
            static void write(const type& val, BinaryBuffer& buffer) {                         \
                auto bytes =                                                                   \
                    std::bit_cast<std::array<uint8_t, sizeof(type)>>(std::byteswap(val));      \
                for (const auto byte : bytes) {                                                \
                    buffer.write_byte(byte);                                                   \
                };                                                                             \
            }                                                                                  \
                                                                                               \
            static type read(BinaryBuffer& buffer) {                                           \
                type value{};                                                                  \
                for (size_t x = 0; x < sizeof(type); x++)                                      \
                    value |= static_cast<type>((buffer.next_byte() << x * 8));                 \
                return std::byteswap(value);                                                   \
            }                                                                                  \
                                                                                               \
            static size_t size(const std::optional<type>& /*unused*/) { return sizeof(type); } \
        };                                                                                     \
    }

INTEGRAL_BINARY(uint16_t);
INTEGRAL_BINARY(int16_t);
INTEGRAL_BINARY(uint32_t);
INTEGRAL_BINARY(int32_t);
INTEGRAL_BINARY(uint64_t);
INTEGRAL_BINARY(int64_t);
INTEGRAL_BINARY(uint8_t);
INTEGRAL_BINARY(int8_t);
INTEGRAL_BINARY(char);
INTEGRAL_BINARY(bool);

namespace rakro {
    template <> struct BinaryDataInterface<std::string> {
        static void write(const std::string& val, BinaryBuffer& buffer) {
            buffer.write(static_cast<uint16_t>(val.size()));
            for (const auto one_char : val) {
                buffer.write_byte(static_cast<uint8_t>(one_char));
            }
        }

        static std::string read(BinaryBuffer& buffer) {
            std::string out;
            auto        cap = buffer.read_next<uint16_t>();
            out.reserve(cap);

            for (size_t x = 0; x < cap; x++) {
                out.push_back(buffer.read_next<char>());
            }

            return out;
        }

        static size_t size(const std::optional<std::string>& string) {
            if (string.has_value()) {
                return string.value().size() + 2;
            } else {
                return 2;
            }
        }
    };
} // namespace rakro

#undef INTEGRAL_BINARY