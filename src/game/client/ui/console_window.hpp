#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace game::ui {

// Satırın türü (renk): normal çıktı, kullanıcının girdiği komut, hata.
enum class ELine { kInfo, kInput, kError };

struct Line_t {
    ELine kind = ELine::kInfo;
    std::string text;
};

// Geliştirici konsolunun durumu. Çizimi doğrudan Dear ImGui ile yapılır (console_window.cpp).
struct Console_t {
    bool open = false;
    bool focused = false;       // true: pencere girdi alır (fare serbest); false: sadece görünür, oyun girdisi çalışır
    bool focusInput = false;    // sonraki frame yazı kutusuna odaklan
    bool scrollToBottom = false;
    std::vector<Line_t> lines;
    std::vector<std::string> history;
    int historyPos = -1;        // -1: yeni satır
    std::array<char, 256> input{};
};

void print(Console_t& console, std::string_view text, ELine kind = ELine::kInfo);
void clear(Console_t& console);

// ImGui penceresini çizer (ImGui frame'i açık olmalı). Enter ile gönderilen satırı verir ("] satır" log'a eklenir).
// Pencere odakta değilse girdi almaz (ImGuiWindowFlags_NoInputs).
[[nodiscard]] std::optional<std::string> draw(Console_t& console);

}  // namespace game::ui
