#pragma once

#include <fstream>
#include <unordered_map>

#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/SystemInterface.h>

namespace game::rml {

// RmlUi'nin sistem hizmetleri: zaman, log (ILog'a), pano (SDL).
class CRmlSystemInterface final : public Rml::SystemInterface {
public:
  double GetElapsedTime() override;
  bool LogMessage(Rml::Log::Type type, const Rml::String &message) override;
  void SetClipboardText(const Rml::String &text) override;
  void GetClipboardText(Rml::String &text) override;
};

// RmlUi dosya erişimi: yollar assets-görelidir ("ui/hud.rml",
// "fonts/DejaVuSans.ttf"); findAssetPath ile çözülür.
class CRmlFileInterface final : public Rml::FileInterface {
public:
  Rml::FileHandle Open(const Rml::String &path) override;
  void Close(Rml::FileHandle file) override;
  size_t Read(void *buffer, size_t size, Rml::FileHandle file) override;
  bool Seek(Rml::FileHandle file, long offset, int origin) override;
  size_t Tell(Rml::FileHandle file) override;

private:
  std::unordered_map<Rml::FileHandle, std::ifstream> m_files;
  Rml::FileHandle m_nextHandle = 1;
};

} // namespace game::rml
