#pragma once

#include "engine/interface/create_interface.hpp"

#include <filesystem>
#include <string_view>

namespace game {

// Dynamic module loader (dlopen / LoadLibrary). Loaded modules stay open until process exit.
class CModule {
public:
    CModule() = default;
    CModule(const CModule&) = delete;
    CModule& operator=(const CModule&) = delete;

    // Log sink bound to every loaded module (the host sets it when it loads log.dll; it goes to the modules' BindLog).
    static void setLogSink(ILog* sink);

    [[nodiscard]] bool load(const std::filesystem::path& file);
    [[nodiscard]] CreateInterfaceFn factory() const { return m_factory; }

    template <typename Interface>
    [[nodiscard]] Interface* getInterface(const char* name) const {
        if (m_factory == nullptr) {
            return nullptr;
        }
        int code = kInterfaceNotFound;
        return static_cast<Interface*>(m_factory(name, &code));
    }

private:
    void* m_handle = nullptr;
    CreateInterfaceFn m_factory = nullptr;
};

// Module file name per platform. Matches the xmake prefix rule exactly:
//   Windows: client.dll      macOS: libclient.dylib      Linux: libclient.so
[[nodiscard]] std::filesystem::path moduleFileName(std::string_view moduleName);

// Host helper: loads log.dll, configures it, and binds it to this process and to modules loaded later.
// Returns nullptr on failure; logs fall back to stderr (warn+) and the host can continue.
[[nodiscard]] ILog* startLogging(CModule& module, const std::filesystem::path& engineDir, const LogConfig_t& config);

}  // namespace game
