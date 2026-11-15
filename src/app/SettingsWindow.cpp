#include "app/SettingsWindow.h"

#include "app/AutoStart.h"
#include "resource.h"

#include <utility>

namespace shot {

SettingsWindow::SettingsWindow(HINSTANCE instance) : instance_(instance) {}

SettingsWindow::~SettingsWindow() {
    if (hwnd_ && ::IsWindow(hwnd_)) ::DestroyWindow(hwnd_);
}

bool SettingsWindow::Create(ApplyCallback callback, HotkeyPreset preset,
                            const std::wstring& status) {
    if (hwnd_) return true;
    onApply_ = std::move(callback);
    preset_ = preset;
    status_ = status;
    hwnd_ = ::CreateDialogParamW(instance_, MAKEINTRESOURCEW(IDD_SETTINGS),
                                 nullptr, &SettingsWindow::DlgProc,
                                 reinterpret_cast<LPARAM>(this));
    return hwnd_ != nullptr;
}

void SettingsWindow::Show() {
    if (!hwnd_) return;
    LoadControls();
    ::ShowWindow(hwnd_, SW_SHOWNORMAL);
    CenterOnScreen();
    ::BringWindowToTop(hwnd_);
    ::SetForegroundWindow(hwnd_);
}

void SettingsWindow::CenterOnScreen() {
    RECT rc{};
    ::GetWindowRect(hwnd_, &rc);
    const int w = rc.right - rc.left;
    const int h = rc.bottom - rc.top;
    RECT work{};
    if (!::SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0)) {
        work.left = 0;
        work.top = 0;
        work.right = ::GetSystemMetrics(SM_CXSCREEN);
        work.bottom = ::GetSystemMetrics(SM_CYSCREEN);
    }
    const int x = work.left + ((work.right - work.left) - w) / 2;
    const int y = work.top + ((work.bottom - work.top) - h) / 2;
    ::SetWindowPos(hwnd_, nullptr, x, y, 0, 0,
                   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void SettingsWindow::Hide() noexcept {
    if (hwnd_) ::ShowWindow(hwnd_, SW_HIDE);
}

bool SettingsWindow::HandleDialogMessage(MSG& msg) const noexcept {
    if (!hwnd_ || !::IsWindowVisible(hwnd_)) return false;
    if (msg.hwnd != hwnd_ && !::IsChild(hwnd_, msg.hwnd)) return false;
    return ::IsDialogMessageW(hwnd_, &msg) != FALSE;
}

void SettingsWindow::SetStatus(const std::wstring& status) {
    status_ = status;
    if (hwnd_) ::SetDlgItemTextW(hwnd_, IDC_STATUS, status_.c_str());
}

INT_PTR CALLBACK SettingsWindow::DlgProc(HWND hwnd, UINT msg,
                                         WPARAM wParam, LPARAM lParam) {
    if (msg == WM_INITDIALOG) {
        auto* self = reinterpret_cast<SettingsWindow*>(lParam);
        ::SetWindowLongPtrW(hwnd, DWLP_USER, reinterpret_cast<LONG_PTR>(self));
    }
    auto* self = reinterpret_cast<SettingsWindow*>(
        ::GetWindowLongPtrW(hwnd, DWLP_USER));
    if (self) return self->OnMessage(hwnd, msg, wParam);
    return FALSE;
}

INT_PTR SettingsWindow::OnMessage(HWND hwnd, UINT msg, WPARAM wParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            hwnd_ = hwnd;
            HICON icon = ::LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON));
            if (icon) {
                ::SendMessageW(hwnd, WM_SETICON, ICON_BIG,
                               reinterpret_cast<LPARAM>(icon));
                ::SendMessageW(hwnd, WM_SETICON, ICON_SMALL,
                               reinterpret_cast<LPARAM>(icon));
            }
            LoadControls();
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    if (onApply_) {
                        const HotkeyPreset preset =
                            (::IsDlgButtonChecked(hwnd, IDC_HOTKEY_SHIFTALT) == BST_CHECKED)
                                ? HotkeyPreset::ShiftAltA : HotkeyPreset::CtrlAltA;
                        const bool autoStart =
                            ::IsDlgButtonChecked(hwnd, IDC_AUTOSTART) == BST_CHECKED;
                        const ApplyResult result = onApply_(preset, autoStart);
                        preset_ = preset;
                        SetStatus(result.status);
                    }
                    return TRUE;
                }
                case IDC_HIDE:
                    Hide();
                    return TRUE;
                case IDCANCEL:
                    Hide();
                    return TRUE;
                default:
                    return FALSE;
            }
        case WM_CLOSE:
            Hide();
            return TRUE;
        default:
            return FALSE;
    }
}

void SettingsWindow::LoadControls() {
    ::SetWindowTextW(hwnd_, L"ScreenShotApp 设置");
    ::SetDlgItemTextW(hwnd_, IDC_HOTKEY_GROUP, L"截图快捷键");
    ::SetDlgItemTextW(hwnd_, IDC_HOTKEY_CTRLALT, L"Ctrl+Alt+A");
    ::SetDlgItemTextW(hwnd_, IDC_HOTKEY_SHIFTALT, L"Shift+Alt+A");
    ::SetDlgItemTextW(hwnd_, IDC_AUTOSTART, L"开机自动启动");
    ::SetDlgItemTextW(hwnd_, IDOK, L"保存");
    ::SetDlgItemTextW(hwnd_, IDC_HIDE, L"隐藏到托盘");

    ::CheckRadioButton(hwnd_, IDC_HOTKEY_CTRLALT, IDC_HOTKEY_SHIFTALT,
                       preset_ == HotkeyPreset::ShiftAltA
                           ? IDC_HOTKEY_SHIFTALT : IDC_HOTKEY_CTRLALT);
    ::CheckDlgButton(hwnd_, IDC_AUTOSTART,
                     AutoStart::IsEnabled() ? BST_CHECKED : BST_UNCHECKED);
    ::SetDlgItemTextW(hwnd_, IDC_STATUS, status_.c_str());
}

}  // namespace shot
