#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace game::ui {

// Line type (color): normal output, a command typed by the user, or an error.
enum class ELine { kInfo, kInput, kError };

struct Line_t {
    ELine kind = ELine::kInfo;
    std::string text;
};

// State of the developer console. It is drawn directly with Dear ImGui (console_window.cpp).
struct Console_t {
    bool open = false;
    bool focused = false;       // true: the window takes input (mouse free); false: only visible, game input keeps working
    bool focusInput = false;    // focus the text box on the next frame
    bool scrollToBottom = false;
    std::vector<Line_t> lines;
    std::vector<std::string> history;
    int historyPos = -1;        // -1: new line
    std::array<char, 256> input{};
};

void print(Console_t& console, std::string_view text, ELine kind = ELine::kInfo);
void clear(Console_t& console);

// Draws the ImGui window (an ImGui frame must be open). Returns the line sent with Enter (it is added to the log as "] line").
// If the window is not focused, it takes no input (ImGuiWindowFlags_NoInputs).
[[nodiscard]] std::optional<std::string> draw(Console_t& console);

}  // namespace game::ui
