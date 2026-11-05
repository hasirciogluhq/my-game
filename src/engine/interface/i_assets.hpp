#pragma once

#include "engine/engine/core/types.hpp"

#include <expected>
#include <memory>
#include <string>

namespace game {

// assets.dll arayüzü. Yol assets/ klasörüne göredir: "models/spot.obj".
// Aynı yol bir kez yüklenir, sonra paylaşılan pointer döner.
class IAssets {
public:
    virtual ~IAssets() = default;
    [[nodiscard]] virtual std::expected<std::shared_ptr<const MeshData_t>, std::string> loadMesh(const std::string& path) = 0;
};

inline constexpr char kAssetsInterface[] = "Assets001";

}  // namespace game
