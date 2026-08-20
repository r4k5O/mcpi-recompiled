#pragma once

namespace mcpi::client {

enum class ScreenKind {
    Title,
    Game,
    Pause,
};

class Screen {
public:
    [[nodiscard]] ScreenKind kind() const noexcept;
    void start_game() noexcept;
    void pause() noexcept;
    void resume() noexcept;
    void to_title() noexcept;

private:
    ScreenKind kind_ = ScreenKind::Title;
};

} // namespace mcpi::client
