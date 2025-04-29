#include <mutex>
#include <optional>
#include <rakro/internal/buffer_company.hpp>

namespace rakro {

    void RentedBuffer::owner_free() noexcept {
        if (!this->owner) {
            return;
        }

        this->owner->add_free(RentedBuffer(this->memory, this->owner));

        this->owner  = nullptr;
        this->memory = {};
    }

    bool BufferCompany::init_block() {
        auto lock = std::unique_lock(this->blocks_mutex);

        if (this->is_making_block) {
            return true;
        }

        this->is_making_block = true;

        if (this->blocks.size() >= this->block_max_count) {
            return false;
        }

        this->blocks.emplace_front(this->buffer_size, this->buffer_count);
        this->is_making_block = false;
        return true;
    }

    void BufferBlock::add_free(RentedBuffer buffer) noexcept {
        const auto lock = std::unique_lock(this->buffer_mutex);
        this->free_buffers.push(std::move(buffer));
        this->free_buffer_count++;
        this->waiter.notify_one();
    }

    RentedBuffer BufferCompany::rent() noexcept {
        const auto try_all = [&] -> std::optional<RentedBuffer> {
            auto lock = std::shared_lock(this->blocks_mutex);
            for (auto& block : this->blocks) {
                auto return_value = block.try_rent();

                if (return_value.has_value()) {
                    return std::move(return_value.value());
                }
            }

            return std::nullopt;
        };

        auto val = try_all();
        if (val.has_value()) {
            return std::move(val.value());
        }

        if (this->blocks.size() == this->block_max_count) {
            return this->blocks.back().rent();
        }

        this->init_block();

        return std::move(try_all().value());
    }

    RentedBuffer BufferBlock::rent() noexcept {
        while (true) {
            auto value = this->try_rent();
            if (value.has_value()) {
                return std::move(value.value());
            }

            auto lock = std::unique_lock(this->buffer_mutex);
            this->waiter.wait(lock, [&] { return this->free_buffers.size() != 0; });
        }
    }

    std::optional<RentedBuffer> BufferBlock::try_rent() noexcept {
        if (this->free_buffer_count == 0) {
            return std::nullopt;
        }

        auto lock = std::unique_lock(this->buffer_mutex);
        if (!this->free_buffers.empty()) {
            this->free_buffer_count--;
            auto buffer = std::move(this->free_buffers.top());
            this->free_buffers.pop();
            return std::optional<RentedBuffer>(std::move(buffer));
        }

        return std::nullopt;
    }

    BufferBlock::BufferBlock(size_t buffer_size, size_t buffer_count) {
        this->free_buffer_count = buffer_count;

        uint8_t* memory = new uint8_t[buffer_size * buffer_count];

        for (size_t start = 0; start < this->free_buffer_count; start++) {
            // Pointer math, BooOooOo scary
            uint8_t* const base_pointer = memory + (start * buffer_size);

            const auto memory_view = std::span<uint8_t>(base_pointer, buffer_size);

            auto buff = RentedBuffer(memory_view, this);
            this->free_buffers.emplace(std::move(buff));
        }
    }
} // namespace rakro