#pragma once

#include <string>
#include <vector>

namespace mcpi::client {

struct UiRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

struct HudLayout {
    float scale = 1.0f;
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    float crosshair_x = 0.0f;
    float crosshair_y = 0.0f;
    UiRect hotbar{};
    UiRect selected_slot{};
};

class HudRenderer {
public:
    [[nodiscard]] HudLayout layout(int width, int height, int selected_slot) const noexcept;

    void push_chat(std::string message, double timestamp);
    [[nodiscard]] std::vector<std::string> visible_chat(double now, double lifetime) const;

private:
    struct ChatLine {
        std::string message;
        double timestamp = 0.0;
    };
    std::vector<ChatLine> chat_;
};

} // namespace mcpi::client
