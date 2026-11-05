#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace game {

// Handle pool with generations. A deleted slot is reused but its generation increases,
// so the old handle does not resolve (get -> nullptr).
// Note: create() may grow the vector; do not use a pointer returned by get() after create().
template <typename Handle, typename T>
class CHandlePool {
public:
    [[nodiscard]] Handle create(T value) {
        uint32_t index = 0;
        if (!m_freeList.empty()) {
            index = m_freeList.back();
            m_freeList.pop_back();
        } else {
            index = static_cast<uint32_t>(m_slots.size());
            m_slots.emplace_back();
        }
        Slot_t& slot = m_slots[index];
        slot.value = std::move(value);
        slot.alive = true;
        ++m_count;
        return Handle{.index = index, .generation = slot.generation};
    }

    [[nodiscard]] T* get(Handle handle) {
        return isAlive(handle) ? &m_slots[handle.index].value : nullptr;
    }

    [[nodiscard]] const T* get(Handle handle) const {
        return isAlive(handle) ? &m_slots[handle.index].value : nullptr;
    }

    // Frees the slot and returns the value (the caller destroys the GPU resource).
    std::optional<T> destroy(Handle handle) {
        if (!isAlive(handle)) {
            return std::nullopt;
        }
        Slot_t& slot = m_slots[handle.index];
        std::optional<T> value{std::move(slot.value)};
        slot.value = T{};
        slot.alive = false;
        slot.generation = slot.generation + 1 == 0 ? 1 : slot.generation + 1;
        m_freeList.push_back(handle.index);
        --m_count;
        return value;
    }

    // Passes every live value to fn and empties the pool (shutdown).
    template <typename Fn>
    void drain(Fn&& fn) {
        for (uint32_t index = 0; index < m_slots.size(); ++index) {
            if (m_slots[index].alive) {
                fn(m_slots[index].value);
            }
        }
        m_slots.clear();
        m_freeList.clear();
        m_count = 0;
    }

    [[nodiscard]] uint32_t count() const { return m_count; }

private:
    struct Slot_t {
        T value{};
        uint32_t generation = 1;
        bool alive = false;
    };

    [[nodiscard]] bool isAlive(Handle handle) const {
        return handle.valid() && handle.index < m_slots.size() && m_slots[handle.index].alive &&
               m_slots[handle.index].generation == handle.generation;
    }

    std::vector<Slot_t> m_slots;
    std::vector<uint32_t> m_freeList;
    uint32_t m_count = 0;
};

}  // namespace game
