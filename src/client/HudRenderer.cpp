#include "client/HudRenderer.hpp"

#include <algorithm>
#include <cstddef>

namespace mcpi::client {

HudLayout HudRenderer::layout(int width, int height, int selected_slot) const noexcept {
    constexpr float reference_width = 848.0f;
    constexpr float reference_height = 480.0f;
    constexpr float hotbar_width = 182.0f;
    constexpr float hotbar_height = 22.0f;
    constexpr float slot_stride = 20.0f;

    const float scale = std::max(0.01f, std::min(
        static_cast<float>(std::max(width, 1)) / reference_width,
        static_cast<float>(std::max(height, 1)) / reference_height));
    const float content_width = reference_width * scale;
    const float content_height = reference_height * scale;
    const float offset_x = (static_cast<float>(width) - content_width) * 0.5f;
    const float offset_y = (static_cast<float>(height) - content_height) * 0.5f;
    const int clamped_slot = std::clamp(selected_slot, 0, 8);

    const float hotbar_x = offset_x + (reference_width - hotbar_width) * 0.5f * scale;
    const float hotbar_y = offset_y + (reference_height - 24.0f) * scale;
    const UiRect hotbar{hotbar_x, hotbar_y, hotbar_width * scale, hotbar_height * scale};
    const UiRect selected{
        hotbar_x + (1.0f + static_cast<float>(clamped_slot) * slot_stride) * scale,
        hotbar_y - scale,
        22.0f * scale,
        24.0f * scale,
    };

    return {
        scale,
        offset_x,
        offset_y,
        offset_x + reference_width * 0.5f * scale,
        offset_y + reference_height * 0.5f * scale,
        hotbar,
        selected,
    };
}

void HudRenderer::push_chat(std::string message, double timestamp) {
    chat_.push_back({std::move(message), timestamp});
    if (chat_.size() > 64U) {
        const auto remove_count = static_cast<std::ptrdiff_t>(chat_.size() - 64U);
        chat_.erase(chat_.begin(), chat_.begin() + remove_count);
    }
}

std::vector<std::string> HudRenderer::visible_chat(double now, double lifetime) const {
    std::vector<std::string> result;
    for (const auto& line : chat_) {
        if (line.timestamp <= now && now - line.timestamp <= lifetime) {
            result.push_back(line.message);
        }
    }
    if (result.size() > 10U) {
        result.erase(result.begin(), result.end() - 10);
    }
    return result;
}

} // namespace mcpi::client
