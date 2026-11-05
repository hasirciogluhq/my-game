#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include "engine/physics/physics_system.hpp"

#include <cstdarg>
#include <unordered_map>

namespace game {
namespace {

constexpr float kCapsuleRadius = kCharacterRadius;  // i_physics.hpp: ışın testi de aynı değerleri kullanır
constexpr float kCapsuleHeight = kCharacterHeight;  // toplam boy (ayaktan tepeye)

// Tek katman: her şey her şeyle çarpışır. Dünya statik olduğu için yeterli.
class SingleBroadPhaseLayer final : public JPH::BroadPhaseLayerInterface {
public:
    JPH::uint GetNumBroadPhaseLayers() const override { return 1; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer) const override { return JPH::BroadPhaseLayer(0); }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer) const override { return "all"; }
#endif
};

class AllObjectVsBroadPhase final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer, JPH::BroadPhaseLayer) const override { return true; }
};

class AllObjectPairs final : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer, JPH::ObjectLayer) const override { return true; }
};

// Jolt varsayılan Trace'i çökertir; sessiz bir tane vermek zorunlu.
void silentTrace(const char*, ...) {}

JPH::RVec3 toJolt(const glm::vec3& v) { return {v.x, v.y, v.z}; }
glm::vec3 fromJolt(JPH::RVec3Arg v) { return {static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ())}; }

}  // namespace

struct CPhysicsSystem::Impl_t {
    SingleBroadPhaseLayer broadPhaseLayers;
    AllObjectVsBroadPhase objectVsBroadPhase;
    AllObjectPairs objectPairs;
    JPH::PhysicsSystem system;
    JPH::TempAllocatorImpl tempAllocator{4 * 1024 * 1024};
    JPH::RefConst<JPH::Shape> capsule;
    std::unordered_map<CharacterHandle_t, JPH::Ref<JPH::CharacterVirtual>> characters;
    CharacterHandle_t nextHandle = 1;
    // Karakter-karakter çarpışması: listedeki karakterler birbirine çarpar. Kapalıyken liste boş.
    JPH::CharacterVsCharacterCollisionSimple characterCollision;
    bool characterCollisionsEnabled = false;
};

CPhysicsSystem::CPhysicsSystem() = default;

CPhysicsSystem::~CPhysicsSystem() {
    shutdown();
}

bool CPhysicsSystem::initialize() {
    if (m_impl != nullptr) {
        return true;
    }
    // Sıra önemli: allocator kaydedilmeden Jolt nesneleri (TempAllocator, PhysicsSystem) yaratılamaz.
    JPH::RegisterDefaultAllocator();
    JPH::Trace = silentTrace;
    // Jolt kendi Factory'sini ham pointer olarak ister (new/delete Jolt API'sinin gereği).
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
    m_impl = std::make_unique<Impl_t>();

    m_impl->system.Init(1024, 0, 1024, 1024, m_impl->broadPhaseLayers, m_impl->objectVsBroadPhase, m_impl->objectPairs);

    // Kapsül: ayak noktası orijin olsun diye yukarı kaydırılır.
    const float cylinderHalf = (kCapsuleHeight - 2.0f * kCapsuleRadius) * 0.5f;
    const JPH::RefConst<JPH::Shape> raw = JPH::CapsuleShapeSettings(cylinderHalf, kCapsuleRadius).Create().Get();
    m_impl->capsule = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, kCapsuleHeight * 0.5f, 0), JPH::Quat::sIdentity(), raw)
                          .Create()
                          .Get();
    return true;
}

void CPhysicsSystem::shutdown() {
    if (m_impl == nullptr) {
        return;
    }
    m_impl.reset();  // Jolt nesneleri Factory'den önce ölmeli
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void CPhysicsSystem::setCharacterCollisions(bool enabled) {
    m_impl->characterCollisionsEnabled = enabled;
    m_impl->characterCollision.mCharacters.clear();
    if (enabled) {
        for (auto& [handle, character] : m_impl->characters) {
            m_impl->characterCollision.Add(character);
        }
    }
}

void CPhysicsSystem::setCharacterPosition(CharacterHandle_t handle, const glm::vec3& feetPosition) {
    const auto it = m_impl->characters.find(handle);
    if (it == m_impl->characters.end()) {
        return;
    }
    it->second->SetPosition(toJolt(feetPosition));
    it->second->SetLinearVelocity(JPH::Vec3::sZero());
}

void CPhysicsSystem::addStaticBox(const glm::vec3& center, const glm::vec3& halfExtents) {
    const JPH::BodyCreationSettings settings(new JPH::BoxShape(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z)),
                                             toJolt(center), JPH::Quat::sIdentity(), JPH::EMotionType::Static, 0);
    m_impl->system.GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::DontActivate);
}

CharacterHandle_t CPhysicsSystem::createCharacter(const glm::vec3& feetPosition) {
    JPH::CharacterVirtualSettings settings;
    settings.mShape = m_impl->capsule;
    settings.mUp = JPH::Vec3::sAxisY();
    // CharacterVirtual Jolt'un RefTarget'ıdır; Ref<> sahipliği alır.
    JPH::Ref<JPH::CharacterVirtual> character = new JPH::CharacterVirtual(&settings, toJolt(feetPosition), JPH::Quat::sIdentity(), 0, &m_impl->system);
    character->SetCharacterVsCharacterCollision(&m_impl->characterCollision);
    if (m_impl->characterCollisionsEnabled) {
        m_impl->characterCollision.Add(character);
    }
    const CharacterHandle_t handle = m_impl->nextHandle++;
    m_impl->characters.emplace(handle, std::move(character));
    return handle;
}

void CPhysicsSystem::destroyCharacter(CharacterHandle_t handle) {
    const auto it = m_impl->characters.find(handle);
    if (it == m_impl->characters.end()) {
        return;
    }
    m_impl->characterCollision.Remove(it->second);
    m_impl->characters.erase(it);
}

CharacterResult_t CPhysicsSystem::moveCharacter(CharacterHandle_t handle, const glm::vec3& velocity, float dt) {
    const auto it = m_impl->characters.find(handle);
    if (it == m_impl->characters.end()) {
        return {};
    }
    JPH::CharacterVirtual& character = *it->second;
    character.SetLinearVelocity(JPH::Vec3(velocity.x, velocity.y, velocity.z));
    character.Update(dt, JPH::Vec3::sZero(),  // yerçekimini oyun kodu uygular
                     m_impl->system.GetDefaultBroadPhaseLayerFilter(0), m_impl->system.GetDefaultLayerFilter(0),
                     {}, {}, m_impl->tempAllocator);
    return {.position = fromJolt(character.GetPosition()),
            .onGround = character.GetGroundState() == JPH::CharacterBase::EGroundState::OnGround};
}

RayHit_t CPhysicsSystem::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
    const glm::vec3 offset = direction * maxDistance;
    const JPH::RRayCast ray(toJolt(origin), JPH::Vec3(offset.x, offset.y, offset.z));
    JPH::RayCastResult result;
    if (!m_impl->system.GetNarrowPhaseQuery().CastRay(ray, result)) {
        return {};
    }
    return {.hit = true, .distance = result.mFraction * maxDistance, .point = origin + offset * result.mFraction};
}

}  // namespace game
