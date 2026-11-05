#include "engine/ui/rml/host.hpp"

#include <chrono>
#include <filesystem>

#include <SDL3/SDL.h>

#include "engine/engine/core/asset_path.hpp"
#include "engine/interface/i_log.hpp"

namespace game::rml {

double CRmlSystemInterface::GetElapsedTime() {
  using clock = std::chrono::steady_clock;
  static const clock::time_point start = clock::now();
  return std::chrono::duration<double>(clock::now() - start).count();
}

bool CRmlSystemInterface::LogMessage(Rml::Log::Type type,
                                     const Rml::String &message) {
  switch (type) {
  case Rml::Log::LT_ERROR:
  case Rml::Log::LT_ASSERT:
    GAME_LOG_ERROR("rml", "{}", message);
    break;
  case Rml::Log::LT_WARNING:
    GAME_LOG_WARN("rml", "{}", message);
    break;
  case Rml::Log::LT_INFO:
  case Rml::Log::LT_ALWAYS:
    GAME_LOG_INFO("rml", "{}", message);
    break;
  default:
    GAME_LOG_DEBUG("rml", "{}", message);
    break;
  }
  return true;
}

void CRmlSystemInterface::SetClipboardText(const Rml::String &text) {
  SDL_SetClipboardText(text.c_str());
}

void CRmlSystemInterface::GetClipboardText(Rml::String &text) {
  char *clipboard =
      SDL_GetClipboardText(); // SDL'in ayırdığı bellek: SDL_free ile bırakılır
  if (clipboard != nullptr) {
    text = clipboard;
    SDL_free(clipboard);
  }
}

Rml::FileHandle CRmlFileInterface::Open(const Rml::String &path) {
  std::filesystem::path resolved =
      findAssetPath(path).value_or(std::filesystem::path{path});
  std::ifstream file{resolved, std::ios::binary};
  if (!file) {
    return 0;
  }
  const Rml::FileHandle handle = m_nextHandle++;
  m_files.emplace(handle, std::move(file));
  return handle;
}

void CRmlFileInterface::Close(Rml::FileHandle file) { m_files.erase(file); }

size_t CRmlFileInterface::Read(void *buffer, size_t size,
                               Rml::FileHandle file) {
  auto it = m_files.find(file);
  if (it == m_files.end()) {
    return 0;
  }
  it->second.read(static_cast<char *>(buffer),
                  static_cast<std::streamsize>(size));
  return static_cast<size_t>(it->second.gcount());
}

bool CRmlFileInterface::Seek(Rml::FileHandle file, long offset, int origin) {
  auto it = m_files.find(file);
  if (it == m_files.end()) {
    return false;
  }
  const std::ios::seekdir direction = origin == SEEK_CUR   ? std::ios::cur
                                      : origin == SEEK_END ? std::ios::end
                                                           : std::ios::beg;
  it->second.clear(); // eof bayrağı seek'i engellemesin
  it->second.seekg(offset, direction);
  return static_cast<bool>(it->second);
}

size_t CRmlFileInterface::Tell(Rml::FileHandle file) {
  auto it = m_files.find(file);
  return it == m_files.end() ? 0 : static_cast<size_t>(it->second.tellg());
}

} // namespace game::rml
