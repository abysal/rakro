
#pragma once
#include <xhash>

namespace rakro {

    template <typename T> class BaseStrongType {
    public:
        auto operator<=>(const BaseStrongType<T>&) const noexcept = default;
        BaseStrongType(T value) : val(value) {}
        BaseStrongType() = default;

        BaseStrongType(const BaseStrongType&)            = default;
        BaseStrongType(BaseStrongType&&)                 = default;
        BaseStrongType& operator=(const BaseStrongType&) = default;
        BaseStrongType& operator=(BaseStrongType&&)      = default;

        T get_val() const noexcept { return val; }

    protected:
        T val{};
    };
} // namespace rakro

namespace std {
    template <typename T> struct hash<rakro::BaseStrongType<T>> {
        size_t operator()(const rakro::BaseStrongType<T>& addr) const noexcept {
            return std::hash<T>{}(addr.get_val());
        }
    };
} // namespace std