
#pragma once
#include "./int24_t.hpp"
#include "buffer_company.hpp"
#include "rakro/internal/buffer_company.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>

namespace rakro {

    //  This class reverses the endian of any int or float wrote

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
            RentedBuffer&& buffer, size_t artificial_limit = static_cast<size_t>(-1)
        )
            : real_buffer(std::forward<RentedBuffer>(buffer)) {

            if (artificial_limit == static_cast<size_t>(-1)) {
                this->bytes = this->real_buffer.get_memory();
            } else {
                this->bytes = this->real_buffer.get_memory().subspan(0, artificial_limit);
            }
        };

        BinaryBuffer()               = default;
        BinaryBuffer(BinaryBuffer&&) = default;
        BinaryBuffer& operator=(BinaryBuffer&& other) noexcept {
            if (this != &other) {
                real_buffer = std::move(other.real_buffer);
                bytes       = std::move(other.bytes);
            }
            return *this;
        }

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

            BinaryDataInterface<T>::write(val, *this);
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

        std::span<uint8_t> remaining_slice() noexcept {
            return this->bytes.subspan(this->index, this->remaining());
        }

        uint8_t* raw() const { return this->bytes.data(); }

        size_t consumed() const { return this->index; }

        size_t clear(size_t limit = static_cast<size_t>(-1)) {
            auto access_index = this->index;
            this->index       = 0;
            if (limit == static_cast<size_t>(-1)) {
                this->bytes = this->real_buffer.get_memory();
            } else {
                this->bytes = this->real_buffer.get_memory().subspan(0, limit);
            }

            return access_index;
        }

        std::span<uint8_t> underlying() { return this->bytes; }

        std::span<uint8_t> consumed_slice() { return this->bytes.subspan(0, this->consumed()); }
        const std::span<const uint8_t> consumed_slice() const {
            return this->bytes.subspan(0, this->consumed());
        }

        void go_to(size_t new_index) noexcept { this->index = new_index; }

        size_t remaining() const noexcept { return this->size() - this->index; }

        size_t size() const noexcept { return this->bytes.size(); }

        void skipn(size_t count) noexcept { this->index += count; }

    private:
        [[maybe_unused]] bool bounds_check(size_t extra) {
            if (this->index + extra - 1 >= bytes.size()) {
                throw std::out_of_range("Not enough space in type");
            }
            return true;
        }

    private:
        std::span<uint8_t> bytes{};
        RentedBuffer       real_buffer{};
        size_t             index{0};
    };
} // namespace rakro

// This impl is actually bugged, but in a way which fixes it soooooooooooooooo, NO ONE TOUCH IT
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
    template <> struct BinaryDataInterface<uint24_t> {
        static void write(const uint24_t& val, BinaryBuffer& buffer) {
            auto bytes = std::bit_cast<std::array<uint8_t, sizeof(uint24_t)>>(val);

            for (size_t x = 1; x < 4; x++) {
                buffer.write_byte(bytes[x]);
            }
        }

        static uint24_t read(BinaryBuffer& buffer) {
            uint32_t bytes{};

            for (size_t x = 0; x < 3; x++) {
                bytes |= uint32_t(buffer.next_byte() << (x * 8));
            }

            return std::bit_cast<uint24_t>(bytes);
        }

        static size_t size(const std ::optional<uint24_t>&) { return 3; }
    };
} // namespace rakro

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