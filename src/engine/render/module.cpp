#include "engine/interface/create_interface.hpp"
#include "engine/render/render_system.hpp"

namespace game {

// Renderer001 is the single renderer interface.
GAME_EXPOSE_INTERFACE(CRenderSystem, IRenderer, kRendererInterface)

}  // namespace game

GAME_EXPORT_MODULE()
