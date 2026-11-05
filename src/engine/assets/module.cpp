#include "engine/assets/asset_system.hpp"
#include "engine/interface/create_interface.hpp"

namespace game {

GAME_EXPOSE_INTERFACE(CAssetSystem, IAssets, kAssetsInterface)

}  // namespace game

GAME_EXPORT_MODULE()
