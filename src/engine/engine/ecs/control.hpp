#pragma once

#include "engine/engine/api.hpp"

#include <entt/entity/registry.hpp>

// Pawn / Controller: oyun kuralı içermeyen, input'tan bağımsız kontrol modeli.
//  - Pawn: kontrol EDİLEN entity (karakter, araç, kamera...).
//  - Controller: kontrol EDEN entity (oyuncu, bot...). Bir anda en çok bir pawn'ı sürer.
//  - Possess: controller pawn'ın başına geçer. Spectate: controller bir pawn'a bakar (view target), sürmez.
//  - Takeover: başka controller'ın sürdüğü pawn'a geçmek. Pawn'ın eski controller'ı pawn'ı geri almak üzere
//    `displaced` olarak hatırlanır; unpossess edilince pawn ona döner (oyuncu bot'u bırakınca bot yürümeye devam eder).
// Pawn/Controller'ın içeriği (input, AI) oyuna aittir; motor sadece bağlantıyı tutar.
namespace game::control {

struct Pawn_t {
    entt::entity controller = entt::null;
    entt::entity displaced = entt::null;  // takeover ile atılan controller; unpossess'te pawn ona döner
};

struct Controller_t {
    entt::entity pawn = entt::null;
    entt::entity viewTarget = entt::null;  // izlenen pawn (boşsa kendi pawn'ı izlenir)
    bool protectsPawn = false;             // true: başka controller bu controller'ın pawn'ını devralamaz
};

ENGINE_API void makePawn(entt::registry& registry, entt::entity entity);
[[nodiscard]] ENGINE_API entt::entity createController(entt::registry& registry, bool protectsPawn);

// controller pawn'ı sürmeye başlar. Önce kendi eski pawn'ını bırakır, izlemeyi keser.
// Pawn başka bir controller'daysa devralır; o controller koruyorsa (protectsPawn) false döner.
ENGINE_API bool possess(entt::registry& registry, entt::entity controller, entt::entity pawn);
// Pawn'ı bırakır. Pawn'ın `displaced` controller'ı hâlâ boştaysa pawn ona döner.
ENGINE_API void unpossess(entt::registry& registry, entt::entity controller);

// Geçersiz / bağlı olmayan durumda entt::null döner.
[[nodiscard]] ENGINE_API entt::entity pawnOf(const entt::registry& registry, entt::entity controller);
[[nodiscard]] ENGINE_API entt::entity controllerOf(const entt::registry& registry, entt::entity pawn);

// Controller bir pawn'a izleyici olur (pawn'ı sürmez). target pawn değilse false.
ENGINE_API bool setViewTarget(entt::registry& registry, entt::entity controller, entt::entity target);
ENGINE_API void clearViewTarget(entt::registry& registry, entt::entity controller);
// Kameranın baktığı pawn: izlenen (hâlâ geçerliyse), yoksa kendi pawn'ı.
[[nodiscard]] ENGINE_API entt::entity viewTarget(const entt::registry& registry, entt::entity controller);

// Pawn ya da controller silinmeden ÖNCE çağrılır (world::destroy yapar): bağlantıları temizler,
// silinen controller'ın pawn'ı `displaced` controller'a döner.
ENGINE_API void onDestroy(entt::registry& registry, entt::entity entity);

}  // namespace game::control
