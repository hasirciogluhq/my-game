#include "engine/interface/module_loader.hpp"

#include <string>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace game {
namespace {

ILog* g_logSink = nullptr;

using BindLogFn = void (*)(ILog*);

}  // namespace

void CModule::setLogSink(ILog* sink) {
    g_logSink = sink;
    logging::bind(sink);
}

bool CModule::load(const std::filesystem::path& file) {
    BindLogFn bindLog = nullptr;
#if defined(_WIN32)
    m_handle = LoadLibraryA(file.string().c_str());
    if (m_handle == nullptr) {
        GAME_LOG_ERROR("module", "failed to load: {}", file.string());
        return false;
    }
    m_factory = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(static_cast<HMODULE>(m_handle), "CreateInterface"));
    bindLog = reinterpret_cast<BindLogFn>(GetProcAddress(static_cast<HMODULE>(m_handle), "BindLog"));
#else
    m_handle = dlopen(file.string().c_str(), RTLD_NOW | RTLD_LOCAL);
    if (m_handle == nullptr) {
        GAME_LOG_ERROR("module", "failed to load: {} ({})", file.string(), dlerror());
        return false;
    }
    m_factory = reinterpret_cast<CreateInterfaceFn>(dlsym(m_handle, "CreateInterface"));
    bindLog = reinterpret_cast<BindLogFn>(dlsym(m_handle, "BindLog"));
#endif
    if (m_factory == nullptr) {
        GAME_LOG_ERROR("module", "CreateInterface not found: {}", file.string());
        return false;
    }
    if (bindLog != nullptr && g_logSink != nullptr) {
        bindLog(g_logSink);
    }
    GAME_LOG_DEBUG("module", "loaded: {}", file.filename().string());
    return true;
}

std::filesystem::path moduleFileName(std::string_view moduleName) {
    const std::string base{moduleName};
#if defined(_WIN32)
    return base + ".dll";
#elif defined(__APPLE__)
    return "lib" + base + ".dylib";
#else
    return "lib" + base + ".so";
#endif
}

ILog* startLogging(CModule& module, const std::filesystem::path& engineDir, const LogConfig_t& config) {
    if (!module.load(engineDir / moduleFileName("log"))) {
        return nullptr;
    }
    auto* sink = module.getInterface<ILog>(kLogInterface);
    if (sink == nullptr) {
        return nullptr;
    }
    sink->configure(config);
    CModule::setLogSink(sink);
    return sink;
}

}  // namespace game
