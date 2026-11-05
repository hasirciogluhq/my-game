#pragma once

#include <string_view>

namespace game {

// Source containing all shaders for each language. New language: shaders/hlsl_source.cpp + a case for it in shaderSource in shader_library.cpp.
[[nodiscard]] std::string_view mslShaderSource();

}  // namespace game
