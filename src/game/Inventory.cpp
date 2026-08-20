#include "game/Inventory.hpp"

#include <algorithm>
#include <stdexcept>

namespace mcpi::game {

Inventory::Inventory() {
    reset_creative_defaults();
}

int Inventory::selected_slot() const noexcept {
    return selected_slot_;
}

void Inventory::select(int slot_index) noexcept {
    selected_slot_ = std::clamp(slot_index, 0, hotbar_size - 1);
}

ItemStack& Inventory::slot(int index) {
    if (index < 0 || index >= hotbar_size) {
        throw std::out_of_range("inventory slot index out of range");
    }
    return slots_[static_cast<std::size_t>(index)];
}

const ItemStack& Inventory::slot(int index) const {
    if (index < 0 || index >= hotbar_size) {
        throw std::out_of_range("inventory slot index out of range");
    }
    return slots_[static_cast<std::size_t>(index)];
}

void Inventory::reset_creative_defaults() noexcept {
    slots_ = {{
        {1, 1, 0},
        {3, 1, 0},
        {4, 1, 0},
        {5, 1, 0},
        {20, 1, 0},
        {45, 1, 0},
        {46, 1, 0},
        {57, 1, 0},
        {89, 1, 0},
    }};
    selected_slot_ = 0;
}

} // namespace mcpi::game
