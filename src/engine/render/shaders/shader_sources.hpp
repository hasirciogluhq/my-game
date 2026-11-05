#pragma once

#include <string_view>

namespace game {

// Dil başına tüm shader'ları içeren kaynak. Yeni dil: shaders/hlsl_source.cpp + shader_library.cpp'deki shaderSource'a case.
[[nodiscard]] std::string_view mslShaderSource();

}  // namespace game
