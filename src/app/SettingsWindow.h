#pragma once

#include "app/HotkeyManager.h"

#include <windows.h>
#include <functional>
#include <string>

namespace shot {

class SettingsWindow {
public:
    struct ApplyResult {
        bool ok = true;
        std::wstring status;
    };
    using ApplyCallback = std::function<ApplyResult(HotkeyPreset, bool)>;

    explicit SettingsWindow(HINSTANCE instance);
    ~SettingsWindow();

    SettingsWindow(const SettingsWindow&) = delete;
    SettingsWindow& operator=(const SettingsWindow&) = delete;

    bool Create(ApplyCallback callback, HotkeyPreset preset,
                const std::wstring& status);
    void Show();
    void Hide() noexcept;
    HWND Handle() const noexcept { return hwnd_; }
    bool HandleDialogMessage(MSG& msg) const noexcept;
    void SetStatus(const std::wstring& status);

private:
    static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR OnMessage(HWND hwnd, UINT msg, WPARAM wParam);
    void LoadControls();
    void CenterOnScreen();

    HINSTANCE instance_;
    HWND hwnd_ = nullptr;
    ApplyCallback onApply_;
    HotkeyPreset preset_ = HotkeyPreset::CtrlAltA;
    std::wstring status_;
};

}  // namespace shot
