#include "game/client/ui/console_window.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstring>

namespace game::ui {

namespace {

ImVec4 colorOf(ELine kind) {
    switch (kind) {
        case ELine::kInput: return {0.55f, 0.85f, 1.0f, 1.0f};
        case ELine::kError: return {1.0f, 0.45f, 0.4f, 1.0f};
        case ELine::kInfo: break;
    }
    return {0.9f, 0.9f, 0.9f, 1.0f};
}

void setInput(ImGuiInputTextCallbackData* data, std::string_view text) {
    data->DeleteChars(0, data->BufTextLen);
    data->InsertChars(0, text.data(), text.data() + text.size());
}

// The console key (` ~) must not be typed into the text; Up/Down browse the command history.
int onInputEvent(ImGuiInputTextCallbackData* data) {
    auto& console = *static_cast<Console_t*>(data->UserData);
    if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
        return (data->EventChar == '`' || data->EventChar == '~') ? 1 : 0;
    }
    const int count = static_cast<int>(console.history.size());
    if (count == 0) {
        return 0;
    }
    if (data->EventKey == ImGuiKey_UpArrow) {
        console.historyPos = console.historyPos < 0 ? count - 1 : std::max(0, console.historyPos - 1);
    } else if (data->EventKey == ImGuiKey_DownArrow && console.historyPos >= 0) {
        ++console.historyPos;
        if (console.historyPos >= count) {
            console.historyPos = -1;
        }
    }
    setInput(data, console.historyPos < 0 ? std::string_view{} : std::string_view{console.history[console.historyPos]});
    return 0;
}

std::optional<std::string> submit(Console_t& console) {
    std::string line{console.input.data()};
    console.input.fill('\0');
    console.historyPos = -1;
    console.focusInput = true;
    if (line.empty()) {
        return std::nullopt;
    }
    console.history.push_back(line);
    print(console, "] " + line, ELine::kInput);
    return line;
}

}  // namespace

void print(Console_t& console, std::string_view text, ELine kind) {
    console.lines.push_back({kind, std::string{text}});
    console.scrollToBottom = true;
}

void clear(Console_t& console) {
    console.lines.clear();
}

std::optional<std::string> draw(Console_t& console) {
    if (!console.open) {
        return std::nullopt;
    }
    ImGui::SetNextWindowSize({640.0f, 320.0f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos({16.0f, 16.0f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.85f);
    const ImGuiWindowFlags flags = console.focused ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoInputs;
    std::optional<std::string> command;
    if (ImGui::Begin("Console", nullptr, flags)) {
        const float inputHeight = ImGui::GetFrameHeightWithSpacing();
        if (ImGui::BeginChild("log", {0.0f, -inputHeight}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
            for (const Line_t& line : console.lines) {
                ImGui::PushStyleColor(ImGuiCol_Text, colorOf(line.kind));
                ImGui::TextWrapped("%s", line.text.c_str());
                ImGui::PopStyleColor();
            }
            if (console.scrollToBottom) {
                ImGui::SetScrollHereY(1.0f);
                console.scrollToBottom = false;
            }
        }
        ImGui::EndChild();

        if (console.focusInput && console.focused) {
            ImGui::SetKeyboardFocusHere();
            console.focusInput = false;
        }
        ImGui::SetNextItemWidth(-1.0f);
        const ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory |
                                               ImGuiInputTextFlags_CallbackCharFilter;
        if (ImGui::InputText("##input", console.input.data(), console.input.size(), inputFlags, onInputEvent, &console)) {
            command = submit(console);
        }
    }
    ImGui::End();
    return command;
}

}  // namespace game::ui
