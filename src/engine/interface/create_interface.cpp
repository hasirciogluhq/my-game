#include "engine/interface/create_interface.hpp"

#include <cstring>
#include <vector>

namespace game {
namespace {

// These statics live as a separate copy in each DLL (the static lib is embedded in every module).
CInterfaceRegistration *&registrationHead() {
  static CInterfaceRegistration *head = nullptr;
  return head;
}

std::vector<CreateInterfaceFn> &moduleFactories() {
  static std::vector<CreateInterfaceFn> factories;
  return factories;
}

} // namespace

CInterfaceRegistration::CInterfaceRegistration(InstanceFactory factory,
                                               const char *name)
    : m_factory(factory), m_name(name), m_next(registrationHead()) {
  registrationHead() = this;
}

void *findLocalInterface(const char *name, int *returnCode) {
  for (const CInterfaceRegistration *node = registrationHead(); node != nullptr;
       node = node->next()) {
    if (std::strcmp(node->name(), name) == 0) {
      if (returnCode != nullptr) {
        *returnCode = kInterfaceOk;
      }
      return node->factory()();
    }
  }
  if (returnCode != nullptr) {
    *returnCode = kInterfaceNotFound;
  }
  return nullptr;
}

void *resolveInterface(const char *name, int *returnCode) {
  for (CreateInterfaceFn factory : moduleFactories()) {
    int code = kInterfaceNotFound;
    if (void *found = factory(name, &code); found != nullptr) {
      if (returnCode != nullptr) {
        *returnCode = kInterfaceOk;
      }
      return found;
    }
  }
  if (returnCode != nullptr) {
    *returnCode = kInterfaceNotFound;
  }
  return nullptr;
}

void registerModuleFactory(CreateInterfaceFn factory) {
  moduleFactories().push_back(factory);
}

} // namespace game
