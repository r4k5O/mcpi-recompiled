#include "assets/FallbackAssetSource.hpp"
#include "client/HudRenderer.hpp"
#include "client/Screen.hpp"
#include "client/SoundEngine.hpp"

#include <cassert>
#include <cmath>

namespace {
bool near(float a, float b, float epsilon = 1.0e-5f) {
    return std::fabs(a - b) <= epsilon;
}
}

int main() {
    using mcpi::assets::FallbackAssetSource;
    using mcpi::client::HudRenderer;
    using mcpi::client::Screen;
    using mcpi::client::ScreenKind;
    using mcpi::client::SoundEngine;

    Screen screen;
    assert(screen.kind() == ScreenKind::Title);
    screen.start_game();
    assert(screen.kind() == ScreenKind::Game);
    screen.pause();
    assert(screen.kind() == ScreenKind::Pause);
    screen.resume();
    assert(screen.kind() == ScreenKind::Game);
    screen.to_title();
    assert(screen.kind() == ScreenKind::Title);

    HudRenderer hud;
    const auto reference = hud.layout(848, 480, 4);
    assert(near(reference.scale, 1.0f));
    assert(near(reference.crosshair_x, 424.0f));
    assert(near(reference.crosshair_y, 240.0f));
    assert(reference.selected_slot.w > 0.0f);
    assert(reference.selected_slot.x > reference.hotbar.x);

    const auto doubled = hud.layout(1696, 960, 4);
    assert(near(doubled.scale, 2.0f));
    assert(near(doubled.crosshair_x, 848.0f));
    assert(near(doubled.crosshair_y, 480.0f));

    hud.push_chat("first", 0.0);
    hud.push_chat("second", 1.0);
    auto visible = hud.visible_chat(2.0, 10.0);
    assert(visible.size() == 2U);
    assert(visible[0] == "first");
    assert(visible[1] == "second");
    visible = hud.visible_chat(10.5, 10.0);
    assert(visible.size() == 1U);
    assert(visible[0] == "second");

    FallbackAssetSource assets;
    SoundEngine sound(assets);
    assert(near(SoundEngine::attenuation({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, 16.0), 1.0f));
    assert(near(SoundEngine::attenuation({0.0, 0.0, 0.0}, {8.0, 0.0, 0.0}, 16.0), 0.5f));
    assert(near(SoundEngine::attenuation({0.0, 0.0, 0.0}, {32.0, 0.0, 0.0}, 16.0), 0.0f));
    assert(!sound.play("missing.event", {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}));

    return 0;
}
