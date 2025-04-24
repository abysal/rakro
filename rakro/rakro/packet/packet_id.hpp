#pragma once
#include <cstdint>

namespace rakro {
    enum class PacketId : uint8_t {
        UnconnectedPing1       = 0x1,
        UnconnectedPing2       = 0x2,
        UnconnectedPong        = 0x1C,
        OpenConnectionRequest1 = 0x5,
        OpenConnectionReply1   = 0x6,
        OpenConnectionRequest2 = 0x7,
        OpenConnectionReply2   = 0x8,
        IncompatibleProtocol   = 0x19
    };
}