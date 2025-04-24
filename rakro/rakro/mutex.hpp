#pragma once
#include <mutex>

namespace rakro {
    template <typename T> class Mutex {
    public:
        explicit Mutex(T&& value) : inner(std::forward<T>(value)) {}

        class LockGuard {

        public:
            LockGuard(T& value, Mutex& owner) : value(value), owner(owner) {}

            ~LockGuard() { this->owner.unlock(); }

            template <typename Self> auto& operator->(this Self self) { return *self.value; }

        private:
            T&     value;
            Mutex& owner;
        };

        LockGuard lock() { return LockGuard(this->inner, *this); }
        void      unlock() { this->inner_lock.unlock(); }

    private:
        T          inner;
        std::mutex inner_lock{};
    };
} // namespace rakro