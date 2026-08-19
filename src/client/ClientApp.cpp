#include "client/ClientApp.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace mcpi::client {
namespace {

constexpr int logical_width = 848;
constexpr int logical_height = 480;
constexpr double pi = 3.14159265358979323846;

struct CameraState {
    double yaw = 0.0;
    double pitch = -0.18;
};

struct ProjectedPoint {
    float x = 0.0f;
    float y = 0.0f;
    float depth = 0.0f;
    bool visible = false;
};

struct RenderQuad {
    std::array<game::Vec3, 4> points{};
    SDL_FColor color{};
    double distance = 0.0;
};

struct RayHit {
    bool found = false;
    game::IVec3 hit{};
    game::IVec3 place{};
};

SDL_FColor block_color(int id, float shade = 1.0f) {
    float r = 0.55f;
    float g = 0.55f;
    float b = 0.55f;

    switch (id) {
    case 1:  r = 0.50f; g = 0.50f; b = 0.52f; break; // stone
    case 2:  r = 0.26f; g = 0.68f; b = 0.22f; break; // grass
    case 3:  r = 0.48f; g = 0.31f; b = 0.18f; break; // dirt
    case 4:  r = 0.42f; g = 0.42f; b = 0.43f; break; // cobble
    case 5:  r = 0.64f; g = 0.48f; b = 0.26f; break; // planks
    case 7:  r = 0.25f; g = 0.25f; b = 0.26f; break; // bedrock
    case 20: r = 0.62f; g = 0.82f; b = 0.90f; break; // glass
    case 41: r = 0.92f; g = 0.74f; b = 0.16f; break; // gold
    case 45: r = 0.66f; g = 0.28f; b = 0.20f; break; // brick
    case 46: r = 0.74f; g = 0.22f; b = 0.18f; break; // TNT
    case 57: r = 0.18f; g = 0.84f; b = 0.86f; break; // diamond
    case 89: r = 0.92f; g = 0.78f; b = 0.34f; break; // glowstone
    default: break;
    }

    return {r * shade, g * shade, b * shade, 1.0f};
}

ProjectedPoint project(const game::Vec3& point,
                       const game::Vec3& eye,
                       const CameraState& camera) {
    const double dx = point.x - eye.x;
    const double dy = point.y - eye.y;
    const double dz = point.z - eye.z;

    const double sin_yaw = std::sin(camera.yaw);
    const double cos_yaw = std::cos(camera.yaw);
    const double side = dx * cos_yaw - dz * sin_yaw;
    const double forward = dx * sin_yaw + dz * cos_yaw;

    const double sin_pitch = std::sin(camera.pitch);
    const double cos_pitch = std::cos(camera.pitch);
    const double vertical = dy * cos_pitch - forward * sin_pitch;
    const double depth = forward * cos_pitch + dy * sin_pitch;

    if (depth <= 0.08) {
        return {};
    }

    constexpr double focal = 430.0;
    return {
        static_cast<float>(logical_width * 0.5 + side * focal / depth),
        static_cast<float>(logical_height * 0.5 - vertical * focal / depth),
        static_cast<float>(depth),
        true,
    };
}

void draw_quad(SDL_Renderer* renderer,
               const RenderQuad& quad,
               const game::Vec3& eye,
               const CameraState& camera) {
    std::array<ProjectedPoint, 4> projected{};
    for (std::size_t index = 0; index < projected.size(); ++index) {
        projected[index] = project(quad.points[index], eye, camera);
        if (!projected[index].visible) {
            return;
        }
    }

    const int order[6] = {0, 1, 2, 0, 2, 3};
    SDL_Vertex vertices[6]{};
    for (int index = 0; index < 6; ++index) {
        const auto& point = projected[static_cast<std::size_t>(order[index])];
        vertices[index].position = {point.x, point.y};
        vertices[index].color = quad.color;
        vertices[index].tex_coord = {0.0f, 0.0f};
    }
    SDL_RenderGeometry(renderer, nullptr, vertices, 6, nullptr, 0);
}

void add_quad(std::vector<RenderQuad>& quads,
              std::array<game::Vec3, 4> points,
              SDL_FColor color,
              const game::Vec3& eye) {
    game::Vec3 center{};
    for (const auto& point : points) {
        center.x += point.x * 0.25;
        center.y += point.y * 0.25;
        center.z += point.z * 0.25;
    }
    const double dx = center.x - eye.x;
    const double dy = center.y - eye.y;
    const double dz = center.z - eye.z;
    quads.push_back({points, color, dx * dx + dy * dy + dz * dz});
}

void render_world(SDL_Renderer* renderer,
                  const game::GameState& game,
                  const CameraState& camera) {
    SDL_SetRenderDrawColor(renderer, 96, 164, 226, 255);
    SDL_RenderClear(renderer);

    const auto player = game.player_position();
    const game::Vec3 eye{player.x, player.y + 1.62, player.z};
    const int center_x = static_cast<int>(std::floor(player.x));
    const int center_z = static_cast<int>(std::floor(player.z));
    constexpr int radius = 18;

    std::vector<RenderQuad> quads;
    quads.reserve(7000);

    for (int x = std::max(0, center_x - radius); x <= std::min(255, center_x + radius); ++x) {
        for (int z = std::max(0, center_z - radius); z <= std::min(255, center_z + radius); ++z) {
            const int height = game.height_at(x, z);
            if (height <= 0) {
                continue;
            }

            const int top_id = game.block_type(x, height - 1, z);
            const SDL_FColor top_color = block_color(top_id, 1.0f);
            add_quad(quads, {{{
                static_cast<double>(x), static_cast<double>(height), static_cast<double>(z)},
                {static_cast<double>(x + 1), static_cast<double>(height), static_cast<double>(z)},
                {static_cast<double>(x + 1), static_cast<double>(height), static_cast<double>(z + 1)},
                {static_cast<double>(x), static_cast<double>(height), static_cast<double>(z + 1)}}},
                top_color, eye);

            const int east = x < 255 ? game.height_at(x + 1, z) : 0;
            const int west = x > 0 ? game.height_at(x - 1, z) : 0;
            const int south = z < 255 ? game.height_at(x, z + 1) : 0;
            const int north = z > 0 ? game.height_at(x, z - 1) : 0;

            if (east < height) {
                add_quad(quads, {{{
                    static_cast<double>(x + 1), static_cast<double>(height), static_cast<double>(z)},
                    {static_cast<double>(x + 1), static_cast<double>(height), static_cast<double>(z + 1)},
                    {static_cast<double>(x + 1), static_cast<double>(east), static_cast<double>(z + 1)},
                    {static_cast<double>(x + 1), static_cast<double>(east), static_cast<double>(z)}}},
                    block_color(top_id, 0.72f), eye);
            }
            if (west < height) {
                add_quad(quads, {{{
                    static_cast<double>(x), static_cast<double>(height), static_cast<double>(z + 1)},
                    {static_cast<double>(x), static_cast<double>(height), static_cast<double>(z)},
                    {static_cast<double>(x), static_cast<double>(west), static_cast<double>(z)},
                    {static_cast<double>(x), static_cast<double>(west), static_cast<double>(z + 1)}}},
                    block_color(top_id, 0.64f), eye);
            }
            if (south < height) {
                add_quad(quads, {{{
                    static_cast<double>(x + 1), static_cast<double>(height), static_cast<double>(z + 1)},
                    {static_cast<double>(x), static_cast<double>(height), static_cast<double>(z + 1)},
                    {static_cast<double>(x), static_cast<double>(south), static_cast<double>(z + 1)},
                    {static_cast<double>(x + 1), static_cast<double>(south), static_cast<double>(z + 1)}}},
                    block_color(top_id, 0.68f), eye);
            }
            if (north < height) {
                add_quad(quads, {{{
                    static_cast<double>(x), static_cast<double>(height), static_cast<double>(z)},
                    {static_cast<double>(x + 1), static_cast<double>(height), static_cast<double>(z)},
                    {static_cast<double>(x + 1), static_cast<double>(north), static_cast<double>(z)},
                    {static_cast<double>(x), static_cast<double>(north), static_cast<double>(z)}}},
                    block_color(top_id, 0.58f), eye);
            }
        }
    }

    std::sort(quads.begin(), quads.end(), [](const RenderQuad& a, const RenderQuad& b) {
        return a.distance > b.distance;
    });
    for (const auto& quad : quads) {
        draw_quad(renderer, quad, eye, camera);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderLine(renderer, logical_width * 0.5f - 7.0f, logical_height * 0.5f,
                   logical_width * 0.5f + 7.0f, logical_height * 0.5f);
    SDL_RenderLine(renderer, logical_width * 0.5f, logical_height * 0.5f - 7.0f,
                   logical_width * 0.5f, logical_height * 0.5f + 7.0f);

    constexpr float slot_width = 34.0f;
    constexpr float slot_gap = 5.0f;
    const float total_width = game::GameState::hotbar_size * (slot_width + slot_gap) - slot_gap;
    const float start_x = (logical_width - total_width) * 0.5f;
    for (int slot = 0; slot < game::GameState::hotbar_size; ++slot) {
        const SDL_FRect outer{start_x + slot * (slot_width + slot_gap), 433.0f, slot_width, slot_width};
        SDL_SetRenderDrawColor(renderer, slot == game.selected_hotbar_slot() ? 255 : 80,
                              slot == game.selected_hotbar_slot() ? 255 : 80,
                              slot == game.selected_hotbar_slot() ? 255 : 80, 235);
        SDL_RenderFillRect(renderer, &outer);

        const SDL_FRect inner{outer.x + 3.0f, outer.y + 3.0f, outer.w - 6.0f, outer.h - 6.0f};
        const auto color = block_color(game.hotbar_block(slot));
        SDL_SetRenderDrawColor(renderer,
                              static_cast<Uint8>(color.r * 255.0f),
                              static_cast<Uint8>(color.g * 255.0f),
                              static_cast<Uint8>(color.b * 255.0f), 255);
        SDL_RenderFillRect(renderer, &inner);
    }

    SDL_RenderPresent(renderer);
}

void render_menu(SDL_Renderer* renderer, bool has_world) {
    SDL_SetRenderDrawColor(renderer, 73, 126, 173, 255);
    SDL_RenderClear(renderer);

    const SDL_FRect shadow{logical_width * 0.5f - 82.0f, 135.0f, 164.0f, 164.0f};
    SDL_SetRenderDrawColor(renderer, 45, 63, 38, 255);
    SDL_RenderFillRect(renderer, &shadow);
    const SDL_FRect dirt{logical_width * 0.5f - 74.0f, 143.0f, 148.0f, 148.0f};
    SDL_SetRenderDrawColor(renderer, 115, 76, 45, 255);
    SDL_RenderFillRect(renderer, &dirt);
    const SDL_FRect grass{logical_width * 0.5f - 74.0f, 143.0f, 148.0f, 34.0f};
    SDL_SetRenderDrawColor(renderer, 64, 170, 58, 255);
    SDL_RenderFillRect(renderer, &grass);

    const SDL_FRect status{logical_width * 0.5f - 125.0f, 330.0f, 250.0f, 12.0f};
    SDL_SetRenderDrawColor(renderer, has_world ? 92 : 176, has_world ? 210 : 110, 76, 255);
    SDL_RenderFillRect(renderer, &status);
    SDL_RenderPresent(renderer);
}

RayHit raycast(const game::GameState& game, const CameraState& camera) {
    const auto player = game.player_position();
    const game::Vec3 eye{player.x, player.y + 1.62, player.z};
    const double cos_pitch = std::cos(camera.pitch);
    const game::Vec3 direction{
        std::sin(camera.yaw) * cos_pitch,
        std::sin(camera.pitch),
        std::cos(camera.yaw) * cos_pitch,
    };

    game::IVec3 previous{
        static_cast<int>(std::floor(eye.x)),
        static_cast<int>(std::floor(eye.y)),
        static_cast<int>(std::floor(eye.z)),
    };

    for (double distance = 0.05; distance <= 6.0; distance += 0.05) {
        const game::Vec3 point{
            eye.x + direction.x * distance,
            eye.y + direction.y * distance,
            eye.z + direction.z * distance,
        };
        const game::IVec3 current{
            static_cast<int>(std::floor(point.x)),
            static_cast<int>(std::floor(point.y)),
            static_cast<int>(std::floor(point.z)),
        };
        if (current.x < 0 || current.x > 255 || current.y < 0 || current.y > 127 || current.z < 0 || current.z > 255) {
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

} // namespace

ClientApp::ClientApp(game::GameState& game, std::mutex& game_mutex, ClientOptions options)
    : game_(game), game_mutex_(game_mutex), options_(std::move(options)) {}

int ClientApp::run() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 20;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Minecraft - Pi edition", logical_width, logical_height,
                                     0, &window, &renderer)) {
        std::cerr << "Could not create Minecraft Pi window: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 21;
    }

    bool running = true;
    bool playing = false;
    CameraState camera;
    Uint64 last_ticks = SDL_GetTicks();

    {
        std::scoped_lock lock(game_mutex_);
        if (std::filesystem::exists(options_.world_path)) {
            game_.load(options_.world_path);
        }
    }

    const auto update_title = [&] {
        bool has_world = false;
        {
            std::scoped_lock lock(game_mutex_);
            has_world = game_.generated_world();
        }
        if (playing) {
            SDL_SetWindowTitle(window,
                "Minecraft - Pi edition | WASD + mouse | LMB break | RMB place | 1-9 hotbar | F5 save | Esc menu");
        } else if (has_world) {
            SDL_SetWindowTitle(window,
                "Minecraft - Pi edition | Enter: Play | N: New World | L: Load World");
        } else {
            SDL_SetWindowTitle(window,
                "Minecraft - Pi edition | N: New World | L: Load World");
        }
    };

    const auto set_playing = [&](bool value) {
        playing = value;
        SDL_SetWindowRelativeMouseMode(window, playing);
        update_title();
    };

    update_title();

    while (running) {
        const Uint64 now = SDL_GetTicks();
        const double dt = std::clamp(static_cast<double>(now - last_ticks) / 1000.0, 0.0, 0.05);
        last_ticks = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                continue;
            }

            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                const SDL_Scancode key = event.key.scancode;
                if (key == SDL_SCANCODE_ESCAPE) {
                    if (playing) {
                        std::scoped_lock lock(game_mutex_);
                        if (game_.generated_world()) {
                            game_.save(options_.world_path);
                        }
                        set_playing(false);
                    } else {
                        running = false;
                    }
                    continue;
                }

                if (!playing) {
                    if (key == SDL_SCANCODE_N) {
                        std::scoped_lock lock(game_mutex_);
                        game_.new_world(options_.seed.value_or(default_seed()));
                        game_.save(options_.world_path);
                        set_playing(true);
                    } else if (key == SDL_SCANCODE_L) {
                        std::scoped_lock lock(game_mutex_);
                        if (game_.load(options_.world_path)) {
                            set_playing(true);
                        }
                    } else if (key == SDL_SCANCODE_RETURN) {
                        std::scoped_lock lock(game_mutex_);
                        if (game_.generated_world()) {
                            set_playing(true);
                        }
                    }
                    continue;
                }

                if (key >= SDL_SCANCODE_1 && key <= SDL_SCANCODE_9) {
                    std::scoped_lock lock(game_mutex_);
                    game_.select_hotbar_slot(static_cast<int>(key - SDL_SCANCODE_1));
                } else if (key == SDL_SCANCODE_F5) {
                    std::scoped_lock lock(game_mutex_);
                    game_.save(options_.world_path);
                } else if (key == SDL_SCANCODE_F9) {
                    std::scoped_lock lock(game_mutex_);
                    game_.load(options_.world_path);
                }
            }

            if (playing && event.type == SDL_EVENT_MOUSE_MOTION) {
                camera.yaw += static_cast<double>(event.motion.xrel) * 0.0025;
                camera.pitch -= static_cast<double>(event.motion.yrel) * 0.0025;
                camera.pitch = std::clamp(camera.pitch, -pi * 0.46, pi * 0.46);
            }

            if (playing && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                std::scoped_lock lock(game_mutex_);
                const RayHit hit = raycast(game_, camera);
                if (hit.found && event.button.button == SDL_BUTTON_LEFT) {
                    game_.break_block(hit.hit.x, hit.hit.y, hit.hit.z);
                    game_.add_block_hit({hit.hit, 1, 0});
                } else if (hit.found && event.button.button == SDL_BUTTON_RIGHT) {
                    game_.place_selected_block(hit.place.x, hit.place.y, hit.place.z);
                }
            }
        }

        if (playing) {
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

            const double sin_yaw = std::sin(camera.yaw);
            const double cos_yaw = std::cos(camera.yaw);
            const game::Vec3 delta{
                (sin_yaw * forward_input + cos_yaw * side_input) * speed * dt,
                vertical_input * speed * dt,
                (cos_yaw * forward_input - sin_yaw * side_input) * speed * dt,
            };

            std::scoped_lock lock(game_mutex_);
            game_.move_player(delta);
            render_world(renderer, game_, camera);
        } else {
            bool has_world = false;
            {
                std::scoped_lock lock(game_mutex_);
                has_world = game_.generated_world();
            }
            render_menu(renderer, has_world);
        }

        SDL_Delay(1);
    }

    {
        std::scoped_lock lock(game_mutex_);
        if (game_.generated_world()) {
            game_.save(options_.world_path);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace mcpi::client
