#include "client/ClientApp.hpp"

#include "assets/FallbackAssetSource.hpp"
#include "assets/OriginalPiAssetSource.hpp"
#include "client/Camera.hpp"
#include "client/HudRenderer.hpp"
#include "client/LevelRenderer.hpp"
#include "client/Screen.hpp"
#include "client/SdlAudioMixer.hpp"
#include "client/SoundEngine.hpp"
#include "storage/StorageRouter.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace mcpi::client {
namespace {

constexpr int logical_width = 848;
constexpr int logical_height = 480;
constexpr double pi = 3.14159265358979323846;

struct Orientation {
    double yaw = 0.0;
    double pitch = -0.18;
};

struct RayHit {
    bool found = false;
    game::IVec3 hit{};
    game::IVec3 place{};
};

class ClientAssetSource final : public assets::AssetSource {
public:
    explicit ClientAssetSource(const std::optional<std::filesystem::path>& requested) {
        if (const auto located = assets::OriginalPiAssetSource::locate(requested)) {
            original_.emplace(*located);
        }
    }

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> read(
        std::string_view path) const override {
        if (original_.has_value()) {
            if (auto value = original_->read(path)) {
                return value;
            }
        }
        return fallback_.read(path);
    }

    [[nodiscard]] bool using_original() const noexcept { return original_.has_value(); }
    [[nodiscard]] std::filesystem::path original_root() const {
        return original_.has_value() ? original_->root() : std::filesystem::path{};
    }

private:
    std::optional<assets::OriginalPiAssetSource> original_;
    assets::FallbackAssetSource fallback_;
};

game::Vec3 look_direction(double yaw, double pitch) {
    const double cos_pitch = std::cos(pitch);
    return {
        std::sin(yaw) * cos_pitch,
        std::sin(pitch),
        std::cos(yaw) * cos_pitch,
    };
}

RayHit raycast(const game::GameState& game, const CameraPose& camera, double reach = 6.0) {
    const auto direction = look_direction(camera.yaw, camera.pitch);
    game::IVec3 previous{
        static_cast<int>(std::floor(camera.position.x)),
        static_cast<int>(std::floor(camera.position.y)),
        static_cast<int>(std::floor(camera.position.z)),
    };

    for (double distance = 0.15; distance <= reach; distance += 0.10) {
        const game::IVec3 current{
            static_cast<int>(std::floor(camera.position.x + direction.x * distance)),
            static_cast<int>(std::floor(camera.position.y + direction.y * distance)),
            static_cast<int>(std::floor(camera.position.z + direction.z * distance)),
        };
        if (current.x < game::GameState::world_min_x || current.x > game::GameState::world_max_x ||
            current.y < game::GameState::world_min_y || current.y > game::GameState::world_max_y ||
            current.z < game::GameState::world_min_z || current.z > game::GameState::world_max_z) {
            previous = current;
            continue;
        }
        if (game.block_type(current.x, current.y, current.z) != 0) {
            return {true, current, previous};
        }
        previous = current;
    }
    return {};
}

std::uint32_t default_seed() {
    const auto value = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return static_cast<std::uint32_t>(value ^ (value >> 32));
}

void draw_rect(SDL_Renderer* renderer, const UiRect& rect, bool filled = true) {
    const SDL_FRect sdl_rect{rect.x, rect.y, rect.w, rect.h};
    if (filled) {
        SDL_RenderFillRect(renderer, &sdl_rect);
    } else {
        SDL_RenderRect(renderer, &sdl_rect);
    }
}

void render_non_game_screen(SDL_Renderer* renderer, ScreenKind kind, bool has_world) {
    if (kind == ScreenKind::Pause) {
        SDL_SetRenderDrawColor(renderer, 35, 40, 48, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, 330.0f, 170.0f, "GAME PAUSED");
        SDL_RenderDebugText(renderer, 258.0f, 215.0f, "ESC / ENTER  Resume");
        SDL_RenderDebugText(renderer, 258.0f, 235.0f, "T            Title screen");
        SDL_RenderDebugText(renderer, 258.0f, 255.0f, "F5           Save world");
        SDL_RenderPresent(renderer);
        return;
    }

    SDL_SetRenderDrawColor(renderer, 95, 163, 225, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_FRect panel{190.0f, 105.0f, 468.0f, 270.0f};
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, 316.0f, 145.0f, "MINECRAFT: PI EDITION");
    SDL_RenderDebugText(renderer, 270.0f, 210.0f, "N       Create new world");
    SDL_RenderDebugText(renderer, 270.0f, 232.0f, "L       Load saved world");
    if (has_world) {
        SDL_RenderDebugText(renderer, 270.0f, 254.0f, "ENTER   Continue world");
    }
    SDL_RenderDebugText(renderer, 270.0f, 300.0f, "ESC     Quit");
    SDL_RenderPresent(renderer);
}

} // namespace

ClientApp::ClientApp(game::GameState& game, std::mutex& game_mutex, ClientOptions options)
    : game_(game), game_mutex_(game_mutex), options_(std::move(options)) {}

int ClientApp::run() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 20;
    }
    const bool audio_available = SDL_InitSubSystem(SDL_INIT_AUDIO);

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Minecraft - Pi edition", logical_width, logical_height,
                                     SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        std::cerr << "Could not create Minecraft Pi window: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 21;
    }

    SDL_Texture* frame_texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
        logical_width, logical_height);
    if (frame_texture == nullptr) {
        std::cerr << "Could not create framebuffer texture: " << SDL_GetError() << '\n';
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 22;
    }

    ClientAssetSource assets(options_.asset_path);
    if (options_.asset_path.has_value() && !assets.using_original()) {
        std::cerr << "Asset directory is not readable: " << options_.asset_path->string()
                  << "; using project-owned fallbacks.\n";
    } else if (assets.using_original()) {
        std::cout << "Using local Minecraft Pi asset tree: " << assets.original_root().string() << '\n';
    }

    SoundEngine sound(assets);
    SdlAudioMixer audio;
    LevelRenderer level_renderer;
    HudRenderer hud;
    Screen screen;
    Orientation orientation;
    bool running = true;
    std::size_t known_chat_messages = 0U;
    Uint64 last_ticks = SDL_GetTicks();

    {
        std::scoped_lock lock(game_mutex_);
        if (std::filesystem::exists(options_.world_path)) {
            (void)storage::load_world(game_, options_.world_path);
        }
    }

    const auto update_window_state = [&] {
        const bool in_game = screen.kind() == ScreenKind::Game;
        SDL_SetWindowRelativeMouseMode(window, in_game);
        if (screen.kind() == ScreenKind::Game) {
            SDL_SetWindowTitle(window,
                "Minecraft - Pi edition | WASD + mouse | LMB break | RMB place | 1-9 hotbar | Esc pause");
        } else if (screen.kind() == ScreenKind::Pause) {
            SDL_SetWindowTitle(window, "Minecraft - Pi edition | Paused | Esc/Enter resume | T title");
        } else {
            SDL_SetWindowTitle(window, "Minecraft - Pi edition | N new | L load | Enter continue | Esc quit");
        }
    };
    update_window_state();

    while (running) {
        const Uint64 ticks = SDL_GetTicks();
        const double dt = std::clamp(static_cast<double>(ticks - last_ticks) / 1000.0, 0.0, 0.05);
        const double now_seconds = static_cast<double>(ticks) / 1000.0;
        last_ticks = ticks;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                continue;
            }

            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                const SDL_Scancode key = event.key.scancode;
                if (screen.kind() == ScreenKind::Title) {
                    if (key == SDL_SCANCODE_ESCAPE) {
                        running = false;
                    } else if (key == SDL_SCANCODE_N) {
                        {
                            std::scoped_lock lock(game_mutex_);
                            game_.new_world(options_.seed.value_or(default_seed()));
                            (void)storage::save_world(game_, options_.world_path);
                        }
                        screen.start_game();
                        update_window_state();
                    } else if (key == SDL_SCANCODE_L) {
                        bool loaded = false;
                        {
                            std::scoped_lock lock(game_mutex_);
                            loaded = storage::load_world(game_, options_.world_path);
                        }
                        if (loaded) {
                            screen.start_game();
                            update_window_state();
                        }
                    } else if (key == SDL_SCANCODE_RETURN) {
                        bool has_world = false;
                        {
                            std::scoped_lock lock(game_mutex_);
                            has_world = game_.generated_world();
                        }
                        if (has_world) {
                            screen.start_game();
                            update_window_state();
                        }
                    }
                    continue;
                }

                if (screen.kind() == ScreenKind::Pause) {
                    if (key == SDL_SCANCODE_ESCAPE || key == SDL_SCANCODE_RETURN) {
                        screen.resume();
                        update_window_state();
                    } else if (key == SDL_SCANCODE_T) {
                        screen.to_title();
                        update_window_state();
                    } else if (key == SDL_SCANCODE_F5) {
                        std::scoped_lock lock(game_mutex_);
                        (void)storage::save_world(game_, options_.world_path);
                    }
                    continue;
                }

                if (key == SDL_SCANCODE_ESCAPE) {
                    {
                        std::scoped_lock lock(game_mutex_);
                        if (game_.generated_world()) {
                            (void)storage::save_world(game_, options_.world_path);
                        }
                    }
                    screen.pause();
                    update_window_state();
                    continue;
                }
                if (key >= SDL_SCANCODE_1 && key <= SDL_SCANCODE_9) {
                    std::scoped_lock lock(game_mutex_);
                    game_.select_hotbar_slot(static_cast<int>(key - SDL_SCANCODE_1));
                } else if (key == SDL_SCANCODE_F5) {
                    std::scoped_lock lock(game_mutex_);
                    (void)storage::save_world(game_, options_.world_path);
                } else if (key == SDL_SCANCODE_F9) {
                    std::scoped_lock lock(game_mutex_);
                    (void)storage::load_world(game_, options_.world_path);
                }
            }

            if (screen.kind() == ScreenKind::Game && event.type == SDL_EVENT_MOUSE_MOTION) {
                orientation.yaw += static_cast<double>(event.motion.xrel) * 0.0025;
                orientation.pitch -= static_cast<double>(event.motion.yrel) * 0.0025;
                orientation.pitch = std::clamp(orientation.pitch, -pi * 0.46, pi * 0.46);
            }

            if (screen.kind() == ScreenKind::Game && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                std::optional<PendingSound> pending;
                {
                    std::scoped_lock lock(game_mutex_);
                    const auto camera = CameraController::resolve(game_, orientation.yaw, orientation.pitch);
                    const RayHit hit = raycast(game_, camera);
                    if (hit.found && event.button.button == SDL_BUTTON_LEFT) {
                        const game::Vec3 origin{
                            static_cast<double>(hit.hit.x) + 0.5,
                            static_cast<double>(hit.hit.y) + 0.5,
                            static_cast<double>(hit.hit.z) + 0.5,
                        };
                        game_.break_block(hit.hit.x, hit.hit.y, hit.hit.z);
                        game_.add_block_hit({hit.hit, 1, 0});
                        if (sound.play("block.break", origin, camera.position)) {
                            pending = sound.take_pending();
                        }
                    } else if (hit.found && event.button.button == SDL_BUTTON_RIGHT) {
                        const game::Vec3 origin{
                            static_cast<double>(hit.place.x) + 0.5,
                            static_cast<double>(hit.place.y) + 0.5,
                            static_cast<double>(hit.place.z) + 0.5,
                        };
                        game_.place_selected_block(hit.place.x, hit.place.y, hit.place.z);
                        if (sound.play("block.place", origin, camera.position)) {
                            pending = sound.take_pending();
                        }
                    }
                }
                if (audio_available && pending.has_value()) {
                    (void)audio.play(*pending);
                }
            }
        }

        if (screen.kind() == ScreenKind::Game) {
            const bool* keys = SDL_GetKeyboardState(nullptr);
            double forward_input = 0.0;
            double side_input = 0.0;
            double vertical_input = 0.0;
            if (keys[SDL_SCANCODE_W]) forward_input += 1.0;
            if (keys[SDL_SCANCODE_S]) forward_input -= 1.0;
            if (keys[SDL_SCANCODE_D]) side_input += 1.0;
            if (keys[SDL_SCANCODE_A]) side_input -= 1.0;
            if (keys[SDL_SCANCODE_SPACE]) vertical_input += 1.0;
            if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) vertical_input -= 1.0;
            const double speed = (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) ? 9.0 : 5.0;
            const double length = std::hypot(forward_input, side_input);
            if (length > 0.0) {
                forward_input /= length;
                side_input /= length;
            }
            const double sin_yaw = std::sin(orientation.yaw);
            const double cos_yaw = std::cos(orientation.yaw);
            const game::Vec3 delta{
                (sin_yaw * forward_input + cos_yaw * side_input) * speed * dt,
                vertical_input * speed * dt,
                (cos_yaw * forward_input - sin_yaw * side_input) * speed * dt,
            };

            RenderFrame frame;
            HudLayout layout;
            std::vector<std::string> chat_lines;
            {
                std::scoped_lock lock(game_mutex_);
                game_.move_player(delta);
                const auto camera = CameraController::resolve(game_, orientation.yaw, orientation.pitch);
                const auto hit = raycast(game_, camera);
                const std::optional<world::BlockPos> selection = hit.found
                    ? std::optional<world::BlockPos>{{hit.hit.x, hit.hit.y, hit.hit.z}}
                    : std::nullopt;
                frame = level_renderer.render(game_.world(), camera, logical_width, logical_height, 1, selection);
                layout = hud.layout(logical_width, logical_height, game_.selected_hotbar_slot());
                const auto& messages = game_.chat_messages();
                while (known_chat_messages < messages.size()) {
                    hud.push_chat(messages[known_chat_messages], now_seconds);
                    ++known_chat_messages;
                }
                chat_lines = hud.visible_chat(now_seconds, 10.0);
            }

            SDL_UpdateTexture(frame_texture, nullptr, frame.rgba.data(), frame.width * 4);
            SDL_RenderTexture(renderer, frame_texture, nullptr, nullptr);

            SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
            draw_rect(renderer, layout.hotbar, true);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            draw_rect(renderer, layout.selected_slot, false);
            SDL_RenderLine(renderer, layout.crosshair_x - 7.0f, layout.crosshair_y,
                           layout.crosshair_x + 7.0f, layout.crosshair_y);
            SDL_RenderLine(renderer, layout.crosshair_x, layout.crosshair_y - 7.0f,
                           layout.crosshair_x, layout.crosshair_y + 7.0f);

            float chat_y = 340.0f;
            for (const auto& line : chat_lines) {
                SDL_RenderDebugText(renderer, 12.0f, chat_y, line.c_str());
                chat_y += 10.0f;
            }
            SDL_RenderPresent(renderer);
        } else {
            bool has_world = false;
            {
                std::scoped_lock lock(game_mutex_);
                has_world = game_.generated_world();
            }
            render_non_game_screen(renderer, screen.kind(), has_world);
        }

        if (audio_available) {
            audio.update();
        }
        SDL_Delay(1);
    }

    {
        std::scoped_lock lock(game_mutex_);
        if (game_.generated_world()) {
            (void)storage::save_world(game_, options_.world_path);
        }
    }

    SDL_DestroyTexture(frame_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace mcpi::client
