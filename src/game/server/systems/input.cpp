#include "game/server/systems/input.hpp"

#include "engine/engine/ecs/control.hpp"

namespace game::systems {

void input(World_t& world) {
    for (auto [controller, queue] : world.registry.view<CommandQueue_t>().each()) {
        // Kuyruk büyümesin (gecikme birikir): sınırı aşan en eski komutlar atılır, kenar bayrakları
        // (basıldı/bırakıldı) bir sonraki komuta eklenir ki zıplama/ateş kaybolmasın.
        while (queue.pending.size() > kMaxCmdBacklog) {
            const UserCmd_t dropped = queue.pending.front();
            queue.pending.pop_front();
            queue.pending.front().buttonsPressed |= dropped.buttonsPressed;
            queue.pending.front().buttonsReleased |= dropped.buttonsReleased;
        }
        if (!queue.pending.empty()) {
            queue.current = queue.pending.front();
            queue.pending.pop_front();
            // client prediction'ın ack'i: komutu şu an uygulanacağı pawn'a yazılır.
            const entt::entity pawn = control::pawnOf(world.registry, controller);
            if (auto* player = pawn != entt::null ? world.registry.try_get<Player_t>(pawn) : nullptr) {
                player->lastCmdSeq = queue.current.tick;
            }
        } else {
            // Paket gelmediyse son komutun hareketini sürdür, tek seferlik edge'leri temizle.
            queue.current.buttonsPressed = 0;
            queue.current.buttonsReleased = 0;
        }
    }
}

}  // namespace game::systems
