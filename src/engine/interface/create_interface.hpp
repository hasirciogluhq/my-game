#pragma once

#include "engine/interface/i_log.hpp"

#if defined(_WIN32)
#define GAME_EXPORT __declspec(dllexport)
#else
#define GAME_EXPORT __attribute__((visibility("default")))
#endif

namespace game {

// Her modülün dışa açtığı tek giriş noktası:  void* CreateInterface(const char*
// name, int* returnCode) Dönen pointer çağıran tarafta arayüz tipine
// static_cast edilir. Bulunamazsa nullptr.
using CreateInterfaceFn = void *(*)(const char *name, int *returnCode);

inline constexpr int kInterfaceOk = 0;
inline constexpr int kInterfaceNotFound = 1;

using InstanceFactory = void *(*)();

// Statik başlatmada kendini modül-içi registry'ye kaydeden düğüm.
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

// Modül tarafı: sadece bu modülde kayıtlı arayüzleri arar.
[[nodiscard]] void *findLocalInterface(const char *name, int *returnCode);

// Tipli sorgu: factory'den arayüzü alır, bulunamazsa nullptr. static_cast'i tek
// yerde toplar.
template <typename Interface>
[[nodiscard]] Interface *queryInterface(CreateInterfaceFn factory,
                                        const char *name) {
  int code = kInterfaceNotFound;
  return factory != nullptr ? static_cast<Interface *>(factory(name, &code))
                            : nullptr;
}

// Host tarafı: yüklenmiş tüm modüllerde (registerModuleFactory ile eklenen)
// arar.
[[nodiscard]] void *resolveInterface(const char *name, int *returnCode);
void registerModuleFactory(CreateInterfaceFn factory);

} // namespace game

// Sınıfı arayüz adıyla kaydeder. ImplClass default constructible olmalı.
// Instance function-local static'tir: modül yaşadığı sürece bir tane vardır,
// ownership yok. Kullanım (modülün kendi namespace'i içinde):
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

// Modülün tek CreateInterface export'u. Her modülün bir .cpp dosyasında bir kez
// çağrılır. BindLog: CModule::load host'un log sink'ini bu modüle bağlar (modül
// kodunda connect() gerekmez).
#define GAME_EXPORT_MODULE()                                                   \
  extern "C" GAME_EXPORT void *CreateInterface(const char *name,               \
                                               int *returnCode) {              \
    return ::game::findLocalInterface(name, returnCode);                       \
  }                                                                            \
  extern "C" GAME_EXPORT void BindLog(::game::ILog *sink) {                    \
    ::game::logging::bind(sink);                                               \
  }
