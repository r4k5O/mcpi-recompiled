#include "client/LevelRenderer.hpp"

#include "world/Chunk.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace mcpi::client {
namespace {

struct ProjectedVertex {
    float x = 0.0f;
    float y = 0.0f;
    float depth = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    float light = 0.0f;
    bool visible = false;
};

struct Triangle {
    std::array<ProjectedVertex, 3> vertices{};
    float average_depth = 0.0f;
};

constexpr std::array<std::uint8_t, 4> sky_color{{105U, 171U, 230U, 255U}};

ProjectedVertex project(const MeshVertex& vertex,
                        const CameraPose& camera,
                        int width,
                        int height) {
    const double dx = static_cast<double>(vertex.x) - camera.position.x;
    const double dy = static_cast<double>(vertex.y) - camera.position.y;
    const double dz = static_cast<double>(vertex.z) - camera.position.z;

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

    const double focal = static_cast<double>(std::min(width, height)) * 0.92;
    return {
        static_cast<float>(static_cast<double>(width) * 0.5 + side * focal / depth),
        static_cast<float>(static_cast<double>(height) * 0.5 - vertical * focal / depth),
        static_cast<float>(depth), vertex.u, vertex.v, vertex.light, true,
    };
}

float edge(float ax, float ay, float bx, float by, float px, float py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

void write_pixel(RenderFrame& frame,
                 std::vector<float>& depth_buffer,
                 int x,
                 int y,
                 float depth,
                 float u,
                 float v,
                 float light,
                 bool translucent,
                 bool write_depth) {
    if (x < 0 || y < 0 || x >= frame.width || y >= frame.height) {
        return;
    }
    const auto index = static_cast<std::size_t>(y * frame.width + x);
    if (depth >= depth_buffer[index]) {
        return;
    }

    const int tile_x = std::clamp(static_cast<int>(std::floor(u * 16.0f)), 0, 15);
    const int tile_y = std::clamp(static_cast<int>(std::floor(v * 16.0f)), 0, 15);
    const float local_u = u * 16.0f - std::floor(u * 16.0f);
    const float local_v = v * 16.0f - std::floor(v * 16.0f);
    const float checker = ((static_cast<int>(local_u * 4.0f) + static_cast<int>(local_v * 4.0f)) & 1) ? 0.86f : 1.0f;
    const float brightness = std::clamp(0.18f + light * 0.82f, 0.18f, 1.0f) * checker;

    float red = static_cast<float>(70 + ((tile_x * 37 + tile_y * 13) % 145)) * brightness;
    float green = static_cast<float>(65 + ((tile_x * 17 + tile_y * 41) % 150)) * brightness;
    float blue = static_cast<float>(70 + ((tile_x * 29 + tile_y * 23) % 145)) * brightness;

    const float fog = std::clamp((depth - 24.0f) / 42.0f, 0.0f, 0.88f);
    red = red * (1.0f - fog) + static_cast<float>(sky_color[0]) * fog;
    green = green * (1.0f - fog) + static_cast<float>(sky_color[1]) * fog;
    blue = blue * (1.0f - fog) + static_cast<float>(sky_color[2]) * fog;

    const auto byte_index = index * 4U;
    if (translucent) {
        constexpr float alpha = 0.55f;
        frame.rgba[byte_index + 0U] = static_cast<std::uint8_t>(
            std::clamp(red * alpha + frame.rgba[byte_index + 0U] * (1.0f - alpha), 0.0f, 255.0f));
        frame.rgba[byte_index + 1U] = static_cast<std::uint8_t>(
            std::clamp(green * alpha + frame.rgba[byte_index + 1U] * (1.0f - alpha), 0.0f, 255.0f));
        frame.rgba[byte_index + 2U] = static_cast<std::uint8_t>(
            std::clamp(blue * alpha + frame.rgba[byte_index + 2U] * (1.0f - alpha), 0.0f, 255.0f));
    } else {
        frame.rgba[byte_index + 0U] = static_cast<std::uint8_t>(std::clamp(red, 0.0f, 255.0f));
        frame.rgba[byte_index + 1U] = static_cast<std::uint8_t>(std::clamp(green, 0.0f, 255.0f));
        frame.rgba[byte_index + 2U] = static_cast<std::uint8_t>(std::clamp(blue, 0.0f, 255.0f));
    }
    frame.rgba[byte_index + 3U] = 255U;
    if (write_depth) {
        depth_buffer[index] = depth;
    }
}

void raster_triangle(RenderFrame& frame,
                     std::vector<float>& depth_buffer,
                     const Triangle& triangle,
                     bool translucent,
                     bool write_depth) {
    const auto& a = triangle.vertices[0];
    const auto& b = triangle.vertices[1];
    const auto& c = triangle.vertices[2];
    if (!a.visible || !b.visible || !c.visible) {
        return;
    }

    const float area = edge(a.x, a.y, b.x, b.y, c.x, c.y);
    if (std::abs(area) < 1.0e-5f) {
        return;
    }

    const int min_x = std::max(0, static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))));
    const int max_x = std::min(frame.width - 1, static_cast<int>(std::ceil(std::max({a.x, b.x, c.x}))));
    const int min_y = std::max(0, static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))));
    const int max_y = std::min(frame.height - 1, static_cast<int>(std::ceil(std::max({a.y, b.y, c.y}))));

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            const float px = static_cast<float>(x) + 0.5f;
            const float py = static_cast<float>(y) + 0.5f;
            const float w0 = edge(b.x, b.y, c.x, c.y, px, py) / area;
            const float w1 = edge(c.x, c.y, a.x, a.y, px, py) / area;
            const float w2 = 1.0f - w0 - w1;
            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) {
                continue;
            }
            const float depth = a.depth * w0 + b.depth * w1 + c.depth * w2;
            const float u = a.u * w0 + b.u * w1 + c.u * w2;
            const float v = a.v * w0 + b.v * w1 + c.v * w2;
            const float light = a.light * w0 + b.light * w1 + c.light * w2;
            write_pixel(frame, depth_buffer, x, y, depth, u, v, light, translucent, write_depth);
        }
    }
}

void append_triangles(const std::vector<MeshVertex>& vertices,
                      const CameraPose& camera,
                      int width,
                      int height,
                      std::vector<Triangle>& output) {
    for (std::size_t offset = 0; offset + 2U < vertices.size(); offset += 3U) {
        Triangle triangle;
        bool visible = true;
        for (std::size_t index = 0; index < 3U; ++index) {
            triangle.vertices[index] = project(vertices[offset + index], camera, width, height);
            visible = visible && triangle.vertices[index].visible;
            triangle.average_depth += triangle.vertices[index].depth / 3.0f;
        }
        if (visible) {
            output.push_back(triangle);
        }
    }
}

void draw_line(RenderFrame& frame, int x0, int y0, int x1, int y1) {
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        if (x0 >= 0 && y0 >= 0 && x0 < frame.width && y0 < frame.height) {
            const auto byte_index = static_cast<std::size_t>(y0 * frame.width + x0) * 4U;
            frame.rgba[byte_index + 0U] = 255U;
            frame.rgba[byte_index + 1U] = 255U;
            frame.rgba[byte_index + 2U] = 255U;
            frame.rgba[byte_index + 3U] = 255U;
        }
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int twice = error * 2;
        if (twice >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

void draw_selection(RenderFrame& frame,
                    const CameraPose& camera,
                    const world::BlockPos& block) {
    std::array<ProjectedVertex, 8> points{};
    for (int index = 0; index < 8; ++index) {
        MeshVertex vertex;
        vertex.x = static_cast<float>(block.x + ((index & 1) ? 1 : 0));
        vertex.y = static_cast<float>(block.y + ((index & 2) ? 1 : 0));
        vertex.z = static_cast<float>(block.z + ((index & 4) ? 1 : 0));
        points[static_cast<std::size_t>(index)] = project(vertex, camera, frame.width, frame.height);
    }
    constexpr std::array<std::array<int, 2>, 12> edges{{
        {{0,1}},{{2,3}},{{4,5}},{{6,7}},{{0,2}},{{1,3}},
        {{4,6}},{{5,7}},{{0,4}},{{1,5}},{{2,6}},{{3,7}},
    }};
    for (const auto& pair : edges) {
        const auto& a = points[static_cast<std::size_t>(pair[0])];
        const auto& b = points[static_cast<std::size_t>(pair[1])];
        if (a.visible && b.visible) {
            draw_line(frame, static_cast<int>(a.x), static_cast<int>(a.y),
                      static_cast<int>(b.x), static_cast<int>(b.y));
        }
    }
}

} // namespace

RenderFrame LevelRenderer::render(const world::World& world,
                                  const CameraPose& camera,
                                  int width,
                                  int height,
                                  int chunk_radius,
                                  std::optional<world::BlockPos> selected) const {
    RenderFrame frame;
    frame.width = std::max(width, 1);
    frame.height = std::max(height, 1);
    frame.rgba.resize(static_cast<std::size_t>(frame.width * frame.height) * 4U);
    for (std::size_t index = 0; index < frame.rgba.size(); index += 4U) {
        frame.rgba[index + 0U] = sky_color[0];
        frame.rgba[index + 1U] = sky_color[1];
        frame.rgba[index + 2U] = sky_color[2];
        frame.rgba[index + 3U] = sky_color[3];
    }
    std::vector<float> depth_buffer(
        static_cast<std::size_t>(frame.width * frame.height),
        std::numeric_limits<float>::infinity());

    const int center_chunk_x = static_cast<int>(std::floor(camera.position.x / world::Chunk::width));
    const int center_chunk_z = static_cast<int>(std::floor(camera.position.z / world::Chunk::depth));
    std::vector<Triangle> opaque;
    std::vector<Triangle> translucent;

    for (int chunk_x = center_chunk_x - chunk_radius; chunk_x <= center_chunk_x + chunk_radius; ++chunk_x) {
        for (int chunk_z = center_chunk_z - chunk_radius; chunk_z <= center_chunk_z + chunk_radius; ++chunk_z) {
            const auto mesh = mesh_builder_.build(world, chunk_x, chunk_z);
            append_triangles(mesh.opaque, camera, frame.width, frame.height, opaque);
            append_triangles(mesh.translucent, camera, frame.width, frame.height, translucent);
        }
    }

    for (const auto& triangle : opaque) {
        raster_triangle(frame, depth_buffer, triangle, false, true);
    }
    std::sort(translucent.begin(), translucent.end(), [](const Triangle& left, const Triangle& right) {
        return left.average_depth > right.average_depth;
    });
    for (const auto& triangle : translucent) {
        raster_triangle(frame, depth_buffer, triangle, true, false);
    }

    if (selected.has_value()) {
        draw_selection(frame, camera, *selected);
    }
    return frame;
}

} // namespace mcpi::client
