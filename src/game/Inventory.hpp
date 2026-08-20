#pragma once

#include <array>

namespace mcpi::game {

struct ItemStack {
    int item_id = 0;
    int count = 0;
    int data = 0;

    bool operator==(const ItemStack&) const = default;
};

class Inventory {
public:
    static constexpr int hotbar_size = 9;

    Inventory();

    [[nodiscard]] int selected_slot() const noexcept;
    void select(int slot) noexcept;

    [[nodiscard]] ItemStack& slot(int index);
    [[nodiscard]] const ItemStack& slot(int index) const;

    void reset_creative_defaults() noexcept;

private:
    std::array<ItemStack, hotbar_size> slots_{};
    int selected_slot_ = 0;
};

} // namespace mcpi::game
