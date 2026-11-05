#pragma once

#include "engine/interface/i_platform.hpp"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace game {

// Oyuncunun yapabildiği aksiyonlar. Fiziksel tuşlar buraya bağlanır (Bindings_t); oyun kodu sadece aksiyonu bilir.
enum class EAction : uint8_t {
    kForward,
    kBack,
    kLeft,
    kRight,
    kAttack,
    kJump,
    kCrouch,
    kUse,
    kReload,
    kWalk,
    kFreeCam,
    kConsole,  // geliştirici konsolu
    kScoreboard,  // basılı tutulunca istatistik tablosu
    kCount,
};

inline constexpr std::size_t kActionCount = static_cast<std::size_t>(EAction::kCount);

// Aksiyon → tuş adı (SDL adı: "W", "Space", "`", "Mouse1"...).
using Bindings_t = std::array<std::string, kActionCount>;

namespace input {

// Aksiyonun config/komut adı ("forward", "attack", "console"...).
[[nodiscard]] std::string_view actionName(EAction action);
[[nodiscard]] std::optional<EAction> actionFromName(std::string_view name);

// Varsayılan tuşlar. Konsol: Esc'in altındaki tuş ("`").
[[nodiscard]] Bindings_t defaultBindings();

// Aksiyonu tuşa bağlar. Tuş adı yanlışsa o aksiyon tetiklenmez (platform bilinmeyen adı yok sayar).
void bind(Bindings_t& bindings, EAction action, std::string key);

// Dosya biçimi: her satır "bind <aksiyon> <tuş>" (tuş adı boşluk içerebilir: "Left Shift").
// Dosya yoksa varsayılanlar kalır.
void load(Bindings_t& bindings, const std::filesystem::path& path);
void save(const Bindings_t& bindings, const std::filesystem::path& path);
// Kayıt/yükleme yeri: assets/config/input.cfg (assets bulunamazsa çalışma dizininde config/input.cfg).
[[nodiscard]] std::filesystem::path configPath();

[[nodiscard]] bool isDown(const IPlatform& platform, const Bindings_t& bindings, EAction action);
[[nodiscard]] bool wasPressed(const IPlatform& platform, const Bindings_t& bindings, EAction action);

}  // namespace input
}  // namespace game
