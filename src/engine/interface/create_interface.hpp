#pragma once

#include "engine/interface/i_log.hpp"

#if defined(_WIN32)
#define GAME_EXPORT __declspec(dllexport)
#else
#define GAME_EXPORT __attribute__((visibility("default")))
#endif

namespace game {

// The single entry point each module exports:  void* CreateInterface(const char*
// name, int* returnCode). The returned pointer is static_cast to the interface type
// on the caller side. nullptr if not found.
using CreateInterfaceFn = void *(*)(const char *name, int *returnCode);

inline constexpr int kInterfaceOk = 0;
inline constexpr int kInterfaceNotFound = 1;

using InstanceFactory = void *(*)();

// Node that registers itself in the module-local registry during static initialization.
class CInterfaceRegistration {
public:
  CInterfaceRegistration(InstanceFactory factory, const char *name);

  [[nodiscard]] InstanceFactory factory() const { return m_factory; }
  [[nodiscard]] const char *name() const { return m_name; }
  [[nodiscard]] const CInterfaceRegistration *next() const { return m_next; }

private:
  InstanceFactory m_factory;
  const char *m_name;
  const CInterfaceRegistration *m_next;
};

// Module side: searches only the interfaces registered in this module.
[[nodiscard]] void *findLocalInterface(const char *name, int *returnCode);

// Typed query: gets the interface from the factory, nullptr if not found.
// Collects the static_cast in one place.
template <typename Interface>
[[nodiscard]] Interface *queryInterface(CreateInterfaceFn factory,
                                        const char *name) {
  int code = kInterfaceNotFound;
  return factory != nullptr ? static_cast<Interface *>(factory(name, &code))
                            : nullptr;
}

// Host side: searches all loaded modules (added with registerModuleFactory).
[[nodiscard]] void *resolveInterface(const char *name, int *returnCode);
void registerModuleFactory(CreateInterfaceFn factory);

} // namespace game

// Registers the class under the interface name. ImplClass must be default constructible.
// The instance is a function-local static: there is exactly one for as long as the module lives, and no ownership.
// Usage (inside the module's own namespace):
// GAME_EXPOSE_INTERFACE(CFoo, IFoo, kFooInterface);
#define GAME_EXPOSE_INTERFACE(ImplClass, InterfaceType, versionName)           \
  namespace {                                                                  \
  void *createInstance##ImplClass() {                                          \
    static ImplClass instance;                                                 \
    InterfaceType *typed = &instance;                                          \
    return typed;                                                              \
  }                                                                            \
  const ::game::CInterfaceRegistration registration##ImplClass{                \
      &createInstance##ImplClass, versionName};                                \
  }

// The module's single CreateInterface export. Called once in one .cpp file of each module.
// BindLog: CModule::load binds the host's log sink to this module (no connect() needed in module code).
#define GAME_EXPORT_MODULE()                                                   \
  extern "C" GAME_EXPORT void *CreateInterface(const char *name,               \
                                               int *returnCode) {              \
    return ::game::findLocalInterface(name, returnCode);                       \
  }                                                                            \
  extern "C" GAME_EXPORT void BindLog(::game::ILog *sink) {                    \
    ::game::logging::bind(sink);                                               \
  }
