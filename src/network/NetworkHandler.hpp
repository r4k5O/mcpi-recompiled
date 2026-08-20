#pragma once

#include "network/Packet.hpp"

namespace mcpi::network {

class NetworkHandler {
public:
    virtual ~NetworkHandler() = default;
    virtual void handle(const PacketFrame& frame) = 0;
};

} // namespace mcpi::network
