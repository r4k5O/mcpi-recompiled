#include "game/EntityRegistry.hpp"

#include <algorithm>

namespace mcpi::game {

bool EntityRegistry::register_external(Entity& entity) noexcept {
    const int id = entity.id();
    if (entities_.contains(id)) {
        return false;
    }
    entities_.emplace(id, &entity);
    return true;
}

int EntityRegistry::add(std::unique_ptr<Entity> entity) {
    if (!entity) {
        return -1;
    }

    const int id = entity->id();
    if (entities_.contains(id)) {
        return -1;
    }

    Entity* raw = entity.get();
    owned_.emplace(id, std::move(entity));
    entities_.emplace(id, raw);
    return id;
}

Entity* EntityRegistry::find(int id) noexcept {
    const auto found = entities_.find(id);
    return found == entities_.end() ? nullptr : found->second;
}

const Entity* EntityRegistry::find(int id) const noexcept {
    const auto found = entities_.find(id);
    return found == entities_.end() ? nullptr : found->second;
}

std::vector<int> EntityRegistry::ids() const {
    std::vector<int> result;
    result.reserve(entities_.size());
    for (const auto& [id, entity] : entities_) {
        (void)entity;
        result.push_back(id);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::size_t EntityRegistry::size() const noexcept {
    return entities_.size();
}

} // namespace mcpi::game
