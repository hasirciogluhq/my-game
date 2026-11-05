#pragma once

#include "engine/interface/create_interface.hpp"

#include <filesystem>
#include <string_view>

namespace game {

// Dinamik modül yükleyici (dlopen / LoadLibrary). Yüklenen modüller süreç sonuna kadar açık kalır.
class CModule {
public:
    CModule() = default;
    CModule(const CModule&) = delete;
    CModule& operator=(const CModule&) = delete;

    // Yüklenen her modüle bağlanacak log sink'i (host log.dll'i yükleyince ayarlar; modüllerin BindLog'una gider).
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

// Platforma göre modül dosya adı. xmake prefix kuralıyla birebir aynıdır:
//   Windows: client.dll      macOS: libclient.dylib      Linux: libclient.so
[[nodiscard]] std::filesystem::path moduleFileName(std::string_view moduleName);

// Host yardımcısı: log.dll'i yükler, yapılandırır, bu process'e ve sonradan yüklenecek modüllere bağlar.
// Başarısızsa nullptr döner; loglar stderr fallback'ine (warn+) düşer, host devam edebilir.
[[nodiscard]] ILog* startLogging(CModule& module, const std::filesystem::path& engineDir, const LogConfig_t& config);

}  // namespace game
