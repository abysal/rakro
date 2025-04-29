#pragma once

#include <cstdint>
#include <xhash>

namespace rakro {

    class uint24_t {
    public:
        constexpr uint24_t() : value(0) {}
        constexpr uint24_t(uint32_t v) : value(v) {}

        constexpr uint32_t get_value() const { return value; }
        constexpr void     set_value(uint32_t v) { value = v & mask; }
        constexpr void     set_value(uint24_t v) { value = v.value; }

        constexpr uint24_t operator+(const uint24_t& other) const {
            return uint24_t((value + other.value));
        }

        constexpr uint24_t operator-(const uint24_t& other) const {
            return uint24_t((value - other.value));
        }

        constexpr uint24_t operator*(const uint24_t& other) const {
            return uint24_t((value * other.value));
        }

        constexpr uint24_t operator/(const uint24_t& other) const {
            return uint24_t((value / other.value));
        }

        constexpr uint24_t operator%(const uint24_t& other) const {
            return uint24_t((value % other.value));
        }

        constexpr bool operator==(const uint24_t& other) const {
            return (value & mask) == (other.value & mask);
        }

        constexpr bool operator!=(const uint24_t& other) const {
            return (value & mask) != (other.value & mask);
        }

        constexpr bool operator<(const uint24_t& other) const {
            return (value & mask) < (other.value & mask);
        }

        constexpr bool operator>(const uint24_t& other) const {
            return (value & mask) > (other.value & mask);
        }

        constexpr bool operator<=(const uint24_t& other) const {
            return (value & mask) <= (other.value & mask);
        }

        constexpr bool operator>=(const uint24_t& other) const {
            return (value & mask) >= (other.value & mask);
        }

        constexpr uint24_t operator&(const uint24_t& other) const {
            return uint24_t(value & other.value);
        }

        constexpr uint24_t operator|(const uint24_t& other) const {
            return uint24_t(value | other.value);
        }

        constexpr uint24_t operator^(const uint24_t& other) const {
            return uint24_t(value ^ other.value);
        }

        constexpr uint24_t operator~() const { return uint24_t(~value); }

        constexpr uint24_t operator<<(int shift) const {
            return uint24_t((value << shift) & mask);
        }

        constexpr uint24_t operator>>(int shift) const { return uint24_t(value >> shift); }

        uint24_t& operator=(uint32_t v) {
            value = v & mask;
            return *this;
        }

        uint24_t& operator+=(const uint24_t& other) {
            value = (value + other.value) & mask;
            return *this;
        }

        uint24_t& operator-=(const uint24_t& other) {
            value = (value - other.value) & mask;
            return *this;
        }

        uint24_t& operator*=(const uint24_t& other) {
            value = (value * other.value) & mask;
            return *this;
        }

        uint24_t& operator/=(const uint24_t& other) {
            value = (value / other.value) & mask;
            return *this;
        }

        uint24_t& operator%=(const uint24_t& other) {
            value = (value % other.value) & mask;
            return *this;
        }

        uint24_t& operator&=(const uint24_t& other) {
            value &= other.value;
            return *this;
        }

        uint24_t& operator|=(const uint24_t& other) {
            value |= other.value;
            return *this;
        }

        uint24_t& operator^=(const uint24_t& other) {
            value ^= other.value;
            return *this;
        }

        uint24_t& operator<<=(int shift) {
            value = (value << shift) & mask;
            return *this;
        }

        uint24_t& operator>>=(int shift) {
            value >>= shift;
            return *this;
        }

        uint24_t& operator++() {
            value = (value + 1) & mask;
            return *this;
        }

        uint24_t operator++(int) {
            uint24_t temp = *this;
            value         = (value + 1) & mask;
            return temp;
        }

        uint24_t& operator--() {
            value = (value - 1) & mask;
            return *this;
        }

        uint24_t operator--(int) {
            uint24_t temp = *this;
            value         = (value - 1) & mask;
            return temp;
        }

    private:
        uint32_t                  value;
        constexpr static uint32_t mask = 0x00FFFFFF;
    };
} // namespace rakro

namespace std {
    template <> struct hash<rakro::uint24_t> {
        size_t operator()(const rakro::uint24_t& addr) const noexcept {
            return std::hash<uint32_t>{}(addr.get_value());
        }
    };
} // namespace std