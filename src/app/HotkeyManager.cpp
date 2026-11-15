#include "app/HotkeyManager.h"

#include "common/Win32Types.h"

namespace shot {

HotkeyManager::Config HotkeyManager::ConfigOf(HotkeyPreset preset) {
    if (preset == HotkeyPreset::ShiftAltA) {
        return Config{ MOD_SHIFT | MOD_ALT, 'A' };
    }
    return Config{ MOD_CONTROL | MOD_ALT, 'A' };
}

bool HotkeyManager::Register(HWND hwnd, HotkeyPreset preset) {
    Unregister(hwnd);
    preset_ = preset;

    const Config alternate = ConfigOf(
        preset == HotkeyPreset::CtrlAltA ? HotkeyPreset::ShiftAltA
                                         : HotkeyPreset::CtrlAltA);
    const Config candidates[] = {
        ConfigOf(preset),
        alternate,
        Config{ MOD_CONTROL | MOD_ALT, 'Q' },
        Config{ MOD_CONTROL | MOD_SHIFT, 'A' },
    };

    for (const Config& candidate : candidates) {
        if (::RegisterHotKey(hwnd, kHotkeyId, candidate.modifiers, candidate.vk)) {
            used_ = candidate;
            registered_ = true;
            return true;
        }
    }

    DebugLog(L"ScreenShotApp: 全局热键全部注册失败，可通过托盘菜单截图\n");
    return false;
}

void HotkeyManager::Unregister(HWND hwnd) noexcept {
    if (!registered_) return;
    ::UnregisterHotKey(hwnd, kHotkeyId);
    registered_ = false;
}

bool HotkeyManager::Matches(WPARAM wParam) const noexcept {
    return registered_ && wParam == kHotkeyId;
}

std::wstring HotkeyManager::Description() const {
    if (!registered_) return L"热键不可用，请使用托盘菜单";
    std::wstring text;
    if (used_.modifiers & MOD_CONTROL) text += L"Ctrl+";
    if (used_.modifiers & MOD_ALT) text += L"Alt+";
    if (used_.modifiers & MOD_SHIFT) text += L"Shift+";
    if (used_.modifiers & MOD_WIN) text += L"Win+";
    text.push_back(static_cast<wchar_t>(used_.vk));
    return text;
}

}  // namespace shot
