#include "game/server/world_tick.hpp"

#include "game/server/systems/ai.hpp"
#include "game/server/systems/death.hpp"
#include "game/server/systems/input.hpp"
#include "game/server/systems/movement.hpp"
#include "game/server/systems/owners.hpp"
#include "game/server/systems/projectiles.hpp"
#include "game/server/systems/weapon.hpp"

namespace game::world {

void tick(World_t& world) {
    systems::input(world);
    systems::ai(world);
    systems::movement(world);
    systems::weapon(world);
    systems::projectiles(world);
    systems::death(world);
    systems::owners(world);
    ++world.tick;
}

}  // namespace game::world
