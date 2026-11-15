#pragma once

#include <windows.h>
#include <string>

namespace shot {

enum class HotkeyPreset {
    CtrlAltA,
    ShiftAltA
};

class HotkeyManager {
public:
    bool Register(HWND hwnd, HotkeyPreset preset);
    void Unregister(HWND hwnd) noexcept;
    bool Matches(WPARAM wParam) const noexcept;
    bool Registered() const noexcept { return registered_; }
    HotkeyPreset Preset() const noexcept { return preset_; }
    std::wstring Description() const;

private:
    struct Config {
        UINT modifiers;
        UINT vk;
    };

    static Config ConfigOf(HotkeyPreset preset);
    static constexpr int kHotkeyId = 1;

    Config used_{ MOD_CONTROL | MOD_ALT, 'A' };
    HotkeyPreset preset_ = HotkeyPreset::CtrlAltA;
    bool registered_ = false;
};

}  // namespace shot
