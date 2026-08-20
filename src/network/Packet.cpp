#include "network/Packet.hpp"

#include <array>
#include <stdexcept>

namespace mcpi::network {

std::vector<std::uint8_t> encode_opaque_frame(const PacketFrame& frame) {
    if (frame.payload.size() + 1U > max_opaque_frame_bytes) {
        throw std::length_error("network frame exceeds evidence-boundary limit");
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(frame.payload.size() + 1U);
    bytes.push_back(frame.id);
    bytes.insert(bytes.end(), frame.payload.begin(), frame.payload.end());
    return bytes;
}

FrameDecodeResult decode_opaque_frame(std::span<const std::uint8_t> bytes) {
    if (bytes.empty()) {
        return {FrameDecodeStatus::EmptyFrame, std::nullopt};
    }
    if (bytes.size() > max_opaque_frame_bytes) {
        return {FrameDecodeStatus::FrameTooLarge, std::nullopt};
    }

    PacketFrame frame;
    frame.id = bytes.front();
    frame.payload.assign(bytes.begin() + 1, bytes.end());
    return {FrameDecodeStatus::Ok, std::move(frame)};
}

std::span<const PacketEvidence> known_packet_evidence() noexcept {
    static constexpr std::array evidence{
        PacketEvidence{"RequestChunkPacket", Reachability::Unknown, std::nullopt, false},
        PacketEvidence{"ChunkDataPacket", Reachability::Unknown, std::nullopt, false},
        PacketEvidence{"MovePlayerPacket", Reachability::Unknown, std::nullopt, false},
        PacketEvidence{"PlaceBlockPacket", Reachability::Unknown, std::nullopt, false},
        PacketEvidence{"UpdateBlockPacket", Reachability::Unknown, std::nullopt, false},
    };
    return evidence;
}

} // namespace mcpi::network
