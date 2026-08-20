#pragma once

#include "game/Entity.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace mcpi::game {

class EntityRegistry {
public:
    // Register an externally owned entity such as GameState's local Player.
    // Returns false for duplicate IDs.
    bool register_external(Entity& entity) noexcept;

    // Takes ownership of an entity. Its existing Entity::id() is the stable
    // registry key. Returns that ID, or -1 for null/duplicate entities.
    int add(std::unique_ptr<Entity> entity);

    [[nodiscard]] Entity* find(int id) noexcept;
    [[nodiscard]] const Entity* find(int id) const noexcept;
    [[nodiscard]] std::vector<int> ids() const;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::unordered_map<int, Entity*> entities_;
    std::unordered_map<int, std::unique_ptr<Entity>> owned_;
};

} // namespace mcpi::game
