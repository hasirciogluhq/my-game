#include "engine/interface/create_interface.hpp"
#include "engine/render/render_system.hpp"

namespace game {

// Renderer001 tek renderer arayüzüdür.
GAME_EXPOSE_INTERFACE(CRenderSystem, IRenderer, kRendererInterface)

}  // namespace game

GAME_EXPORT_MODULE()
