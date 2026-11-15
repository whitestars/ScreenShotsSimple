#include "app/Settings.h"

namespace shot::Settings {
namespace {

constexpr wchar_t kSettingsKey[] = L"Software\\ScreenShotApp";
constexpr wchar_t kHotkeyValue[] = L"Hotkey";

}  // namespace

HotkeyPreset LoadHotkey() {
    HKEY key = nullptr;
    if (::RegOpenKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, KEY_QUERY_VALUE, &key)
        != ERROR_SUCCESS) {
        return HotkeyPreset::CtrlAltA;
    }

    DWORD value = 0;
    DWORD size = sizeof(value);
    DWORD type = 0;
    const LONG result = ::RegQueryValueExW(
        key, kHotkeyValue, nullptr, &type,
        reinterpret_cast<BYTE*>(&value), &size);
    ::RegCloseKey(key);

    if (result != ERROR_SUCCESS || type != REG_DWORD || value > 1) {
        return HotkeyPreset::CtrlAltA;
    }
    return static_cast<HotkeyPreset>(value);
}

bool SaveHotkey(HotkeyPreset preset) {
    HKEY key = nullptr;
    if (::RegCreateKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, nullptr, 0,
                          KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }

    const DWORD value = static_cast<DWORD>(preset);
    const LONG result = ::RegSetValueExW(
        key, kHotkeyValue, 0, REG_DWORD,
        reinterpret_cast<const BYTE*>(&value), sizeof(value));
    ::RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

}  // namespace shot::Settings
