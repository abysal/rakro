#pragma once
#include <cstdint>

namespace rakro {
    enum class PacketId : uint8_t {
        ConnectedPingPong         = 0x0,
        UnconnectedPing1          = 0x1,
        UnconnectedPing2          = 0x2,
        UnconnectedPong           = 0x1C,
        OpenConnectionRequest1    = 0x5,
        OpenConnectionReply1      = 0x6,
        OpenConnectionRequest2    = 0x7,
        OpenConnectionReply2      = 0x8,
        ConnectionRequest         = 0x9,
        ConnectionRequestAccepted = 0x10,
        NewIncommingConnection    = 0x13,
        IncompatibleProtocol      = 0x19,
        Ack                       = 0xC0,
        Nack                      = 0xA0
    };
}