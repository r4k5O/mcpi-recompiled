#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace mcpi::network {

struct PacketFrame {
    std::uint8_t id = 0;
    std::vector<std::uint8_t> payload;

    bool operator==(const PacketFrame&) const = default;
};

inline constexpr std::size_t max_opaque_frame_bytes = 1024U * 1024U;

enum class FrameDecodeStatus {
    Ok,
    EmptyFrame,
    FrameTooLarge,
};

struct FrameDecodeResult {
    FrameDecodeStatus status = FrameDecodeStatus::EmptyFrame;
    std::optional<PacketFrame> frame;
};

std::vector<std::uint8_t> encode_opaque_frame(const PacketFrame& frame);
FrameDecodeResult decode_opaque_frame(std::span<const std::uint8_t> bytes);

enum class Reachability {
    Reachable,
    Dormant,
    Unknown,
};

struct PacketEvidence {
    std::string_view rtti_name;
    Reachability reachability = Reachability::Unknown;
    std::optional<std::uint8_t> wire_id;
    bool payload_layout_confirmed = false;
};

std::span<const PacketEvidence> known_packet_evidence() noexcept;

} // namespace mcpi::network
