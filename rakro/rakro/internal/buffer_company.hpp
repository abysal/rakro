#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <span>
#include <stack>
#include <utility>
namespace rakro {

    // TODO: If contention is a massive issue, use a lockfree queue to store free buffers

    class RentedBuffer {
    public:
        RentedBuffer()                    = default;
        RentedBuffer(const RentedBuffer&) = delete;
        RentedBuffer(std::span<uint8_t> memory, struct BufferBlock* owner)
            : memory(memory), owner(owner) {}
        RentedBuffer(RentedBuffer&& other) noexcept {
            this->owner_free();
            this->owner  = std::exchange(other.owner, nullptr);
            this->memory = std::exchange(other.memory, {});
        }

        RentedBuffer& operator=(RentedBuffer&& other) noexcept {
            if (this != &other) {
                this->owner_free();
                this->owner  = std::exchange(other.owner, nullptr);
                this->memory = std::exchange(other.memory, {});
            }
            return *this;
        }

        ~RentedBuffer() noexcept {
            if (this->owner && this->memory.size() != 0) {
                this->owner_free();
            }
        }

        std::span<uint8_t> get_memory() noexcept { return this->memory; }

    private:
        void owner_free() noexcept;

    private:
        std::span<uint8_t>  memory{};
        struct BufferBlock* owner{nullptr};
    };

    struct BufferBlock {
        std::stack<RentedBuffer> free_buffers{};
        std::mutex               buffer_mutex{};
        std::condition_variable  waiter{};
        uint8_t*                 block_memory{nullptr};
        std::atomic_size_t       free_buffer_count{};

        BufferBlock(size_t buffer_size, size_t buffer_count);

        void add_free(RentedBuffer buffer) noexcept;

        std::optional<RentedBuffer> try_rent() noexcept;
        // This will always return a valid buffer, but it may sleep for a long time
        RentedBuffer rent() noexcept;
    };

    class BufferCompany {
    public:
        BufferCompany(
            size_t buffer_count = 512, size_t buffer_size = 2048, size_t block_max_count = 4
        )
            : buffer_count(buffer_count), buffer_size(buffer_size),
              block_max_count(block_max_count) {
            this->init_block();
        }

        RentedBuffer rent() noexcept;

    private:
        bool init_block();

    private:
        size_t                 buffer_count{512};
        size_t                 buffer_size{2048};
        size_t                 block_max_count{4};
        std::shared_mutex      blocks_mutex{};
        std::list<BufferBlock> blocks{};
        std::atomic_bool       is_making_block{false};
    };

} // namespace rakro