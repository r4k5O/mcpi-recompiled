#include "client/Screen.hpp"

namespace mcpi::client {

ScreenKind Screen::kind() const noexcept { return kind_; }
void Screen::start_game() noexcept { kind_ = ScreenKind::Game; }
void Screen::pause() noexcept { if (kind_ == ScreenKind::Game) kind_ = ScreenKind::Pause; }
void Screen::resume() noexcept { if (kind_ == ScreenKind::Pause) kind_ = ScreenKind::Game; }
void Screen::to_title() noexcept { kind_ = ScreenKind::Title; }

} // namespace mcpi::client
