#include "network/NetworkHandler.hpp"
#include "network/Packet.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace {

class RecordingNetworkHandler final : public mcpi::network::NetworkHandler {
public:
    void handle(const mcpi::network::PacketFrame& frame) override {
        frames.push_back(frame);
    }

    std::vector<mcpi::network::PacketFrame> frames;
};

} // namespace

int main() {
    using namespace mcpi::network;

    // The reconstruction boundary accepts one already-demarcated packet at a
    // time. Empty input and unreasonably large opaque frames are rejected
    // before any packet-specific interpretation happens.
    const auto empty = decode_opaque_frame({});
    assert(empty.status == FrameDecodeStatus::EmptyFrame);
    assert(!empty.frame.has_value());

    std::vector<std::uint8_t> too_large(max_opaque_frame_bytes + 1U, 0x5aU);
    const auto oversized = decode_opaque_frame(too_large);
    assert(oversized.status == FrameDecodeStatus::FrameTooLarge);
    assert(!oversized.frame.has_value());

    // Unknown IDs must be preserved, not assigned speculative MCPE semantics.
    const PacketFrame unknown{0xfeU, {0x00U, 0x7fU, 0xffU, 0x42U}};
    const auto encoded = encode_opaque_frame(unknown);
    assert(encoded.size() == unknown.payload.size() + 1U);
    assert(encoded.front() == unknown.id);

    const auto decoded = decode_opaque_frame(encoded);
    assert(decoded.status == FrameDecodeStatus::Ok);
    assert(decoded.frame.has_value());
    assert(decoded.frame->id == unknown.id);
    assert(decoded.frame->payload == unknown.payload);

    RecordingNetworkHandler handler;
    handler.handle(*decoded.frame);
    assert(handler.frames.size() == 1U);
    assert(handler.frames.front().id == 0xfeU);
    assert(handler.frames.front().payload == unknown.payload);

    // These names are direct RTTI evidence from the supplied Pi 0.1.1 ELF.
    // Wire IDs/layouts and runtime reachability remain unknown until separately
    // evidenced; the API must make that uncertainty explicit.
    const auto evidence = known_packet_evidence();
    constexpr std::string_view expected_names[] = {
        "RequestChunkPacket",
        "ChunkDataPacket",
        "MovePlayerPacket",
        "PlaceBlockPacket",
        "UpdateBlockPacket",
    };

    for (const auto name : expected_names) {
        const auto it = std::find_if(evidence.begin(), evidence.end(), [name](const PacketEvidence& entry) {
            return entry.rtti_name == name;
        });
        assert(it != evidence.end());
        assert(it->reachability == Reachability::Unknown);
        assert(!it->wire_id.has_value());
        assert(!it->payload_layout_confirmed);
    }

    return 0;
}
