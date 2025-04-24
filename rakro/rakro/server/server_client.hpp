#pragma once

#include <cstdint>
namespace rakro {

    class RakroServerClient {};

    struct SemiConnectedClient {
        uint64_t connection_time{};
        uint16_t mtu{};
    };
} // namespace rakro