#include "app/Application.h"

#include "app/AutoStart.h"
#include "app/Settings.h"
#include "capture/ImageEncoder.h"
#include "common/Win32Types.h"
#include "resource.h"

#include <shellapi.h>

#include <cwchar>
#include <string>
#include <utility>

namespace shot {
namespace {

constexpr wchar_t kMsgClassName[] = L"ScreenShotApp_MsgHost";
constexpr UINT kTrayId = 1;
constexpr UINT kMenuScreenshot = 100;
constexpr UINT kMenuQuit = 101;
constexpr UINT kMenuSettings = 102;

std::wstring TrayTip(const std::wstring& hotkey) {
    return L"ScreenShotApp 运行中（" + hotkey + L" 截图）";
}

}  // namespace

Application::Application(HINSTANCE instance) : instance_(instance) {}

Application::~Application() {
    settings_.reset();
    if (msgHwnd_ && ::IsWindow(msgHwnd_)) ::DestroyWindow(msgHwnd_);
}

bool Application::CreateMessageWindow() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &Application::WndProc;
    wc.hInstance = instance_;
    wc.hIcon = ::LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hIconSm = wc.hIcon;
    wc.lpszClassName = kMsgClassName;
    if (!::RegisterClassExW(&wc) && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    msgHwnd_ = ::CreateWindowExW(0, kMsgClassName, L"ScreenShotApp", 0,
                                 0, 0, 0, 0, nullptr, nullptr, instance_, this);
    return msgHwnd_ != nullptr;
}

bool Application::AddTrayIcon() {
    NOTIFYICONDATAW icon{};
    icon.cbSize = sizeof(icon);
    icon.hWnd = msgHwnd_;
    icon.uID = kTrayId;
    icon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    icon.uCallbackMessage = kAppMsgTray;
    icon.hIcon = ::LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON));
    if (!icon.hIcon) icon.hIcon = ::LoadIconW(nullptr, IDI_APPLICATION);

    const std::wstring tip = TrayTip(hotkey_.Description());
    std::wcsncpy(icon.szTip, tip.c_str(),
                 sizeof(icon.szTip) / sizeof(icon.szTip[0]) - 1);

    trayAdded_ = ::Shell_NotifyIconW(NIM_ADD, &icon) != FALSE;
    if (!trayAdded_) DebugLog(L"ScreenShotApp: 托盘图标创建失败\n");
    return trayAdded_;
}

void Application::ShowTrayNotice() {
    if (!trayAdded_) return;
    NOTIFYICONDATAW notice{};
    notice.cbSize = sizeof(notice);
    notice.hWnd = msgHwnd_;
    notice.uID = kTrayId;
    notice.uFlags = NIF_INFO;
    notice.dwInfoFlags = NIIF_INFO;
    std::wcsncpy(notice.szInfoTitle, L"ScreenShotApp",
                 sizeof(notice.szInfoTitle) / sizeof(notice.szInfoTitle[0]) - 1);
    std::wcsncpy(notice.szInfo, L"已后台运行，按热键或右键托盘图标截图",
                 sizeof(notice.szInfo) / sizeof(notice.szInfo[0]) - 1);
    ::Shell_NotifyIconW(NIM_MODIFY, &notice);
}

void Application::RefreshTrayTip() {
    if (!trayAdded_) return;
    NOTIFYICONDATAW icon{};
    icon.cbSize = sizeof(icon);
    icon.hWnd = msgHwnd_;
    icon.uID = kTrayId;
    icon.uFlags = NIF_TIP;
    const std::wstring tip = TrayTip(hotkey_.Description());
    std::wcsncpy(icon.szTip, tip.c_str(),
                 sizeof(icon.szTip) / sizeof(icon.szTip[0]) - 1);
    ::Shell_NotifyIconW(NIM_MODIFY, &icon);
}

void Application::RemoveTrayIcon() noexcept {
    if (!trayAdded_) return;
    NOTIFYICONDATAW icon{};
    icon.cbSize = sizeof(icon);
    icon.hWnd = msgHwnd_;
    icon.uID = kTrayId;
    ::Shell_NotifyIconW(NIM_DELETE, &icon);
    trayAdded_ = false;
}

void Application::CreateSettingsWindow(bool show) {
    settings_ = std::make_unique<SettingsWindow>(instance_);
    const std::wstring status = L"当前生效：" + hotkey_.Description();
    if (!settings_->Create(
            [this](HotkeyPreset preset, bool autoStart) {
                return ApplySettings(preset, autoStart);
            },
            preset_, status)) {
        DebugLog(L"ScreenShotApp: 创建设置窗口失败\n");
        settings_.reset();
        return;
    }
    if (show) settings_->Show();
}

SettingsWindow::ApplyResult Application::ApplySettings(HotkeyPreset preset,
                                                       bool autoStart) {
    SettingsWindow::ApplyResult result;
    const bool saved = Settings::SaveHotkey(preset);
    preset_ = preset;
    hotkey_.Unregister(msgHwnd_);
    const bool registered = hotkey_.Register(msgHwnd_, preset_);
    const bool autoStartOk = AutoStart::SetEnabled(autoStart);
    RefreshTrayTip();

    result.status = L"当前生效：" + hotkey_.Description();
    if (!autoStartOk) result.status += L"（自启设置失败）";
    result.ok = saved && registered && autoStartOk;
    return result;
}

bool Application::Initialize(bool showSettings) {
    if (!CreateMessageWindow()) {
        DebugLog(L"ScreenShotApp: 创建消息窗口失败\n");
        return false;
    }
    preset_ = Settings::LoadHotkey();
    if (!hotkey_.Register(msgHwnd_, preset_)) {
        DebugLog(L"ScreenShotApp: 热键未注册，仍可通过托盘截图\n");
    }
    AddTrayIcon();
    CreateSettingsWindow(showSettings);
    if (!showSettings) ShowTrayNotice();
    return true;
}

int Application::Run() {
    MSG msg{};
    while (::GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (settings_ && settings_->HandleDialogMessage(msg)) continue;
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK Application::WndProc(HWND hwnd, UINT msg,
                                      WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                            reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }
    auto* self = reinterpret_cast<Application*>(
        ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) return self->OnMessage(hwnd, msg, wParam, lParam);
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT Application::OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_HOTKEY:
            if (hotkey_.Matches(wParam)) StartSession();
            return 0;
        case kAppMsgStart:
            StartSession();
            return 0;
        case kAppMsgConfirm:
            Confirm();
            return 0;
        case kAppMsgSave:
            Save();
            return 0;
        case kAppMsgEndSession:
            EndSession();
            return 0;
        case kAppMsgQuit:
            Quit();
            return 0;
        case kAppMsgSettings:
            ShowSettings();
            return 0;
        case kAppMsgTray:
            if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_CONTEXTMENU) {
                ShowTrayMenu();
            } else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
                ShowSettings();
            }
            return 0;
        case WM_QUERYENDSESSION:
            return TRUE;
        case WM_DESTROY:
            RemoveTrayIcon();
            hotkey_.Unregister(msgHwnd_);
            ::PostQuitMessage(0);
            return 0;
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void Application::ShowTrayMenu() {
    HMENU menu = ::CreatePopupMenu();
    if (!menu) return;
    ::AppendMenuW(menu, MF_STRING, kMenuScreenshot, L"截图");
    ::AppendMenuW(menu, MF_STRING, kMenuSettings, L"设置");
    ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(menu, MF_STRING, kMenuQuit, L"退出");

    POINT cursor{};
    ::GetCursorPos(&cursor);
    ::SetForegroundWindow(msgHwnd_);
    const UINT command = static_cast<UINT>(::TrackPopupMenu(
        menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        cursor.x, cursor.y, 0, msgHwnd_, nullptr));
    ::DestroyMenu(menu);

    if (command == kMenuScreenshot) StartSession();
    else if (command == kMenuSettings) ShowSettings();
    else if (command == kMenuQuit) Quit();
}

void Application::ShowSettings() {
    if (inSession_) EndSession();
    if (!settings_) {
        CreateSettingsWindow(true);
        return;
    }
    settings_->Show();
}

void Application::StartSession() {
    if (inSession_) return;
    if (!image_.Capture()) return;

    stack_.Clear();
    overlay_ = std::make_unique<OverlayWindow>(instance_, image_, stack_);

    OverlayWindow::Callbacks callbacks;
    callbacks.onConfirm = [this] {
        ::PostMessageW(msgHwnd_, kAppMsgConfirm, 0, 0);
    };
    callbacks.onSave = [this] {
        ::PostMessageW(msgHwnd_, kAppMsgSave, 0, 0);
    };
    callbacks.onCancel = [this] {
        ::PostMessageW(msgHwnd_, kAppMsgEndSession, 0, 0);
    };
    overlay_->SetCallbacks(std::move(callbacks));

    if (!overlay_->Show()) {
        overlay_.reset();
        stack_.Clear();
        image_.Release();
        return;
    }
    inSession_ = true;
}

void Application::EndSession() {
    inSession_ = false;
    overlay_.reset();
    stack_.Clear();
    image_.Release();
    ::SetProcessWorkingSetSize(::GetCurrentProcess(),
                               static_cast<SIZE_T>(-1),
                               static_cast<SIZE_T>(-1));
}

void Application::Confirm() {
    if (!inSession_ || !overlay_) return;

    const RECT selection = overlay_->Selection();
    ImageEncoder::BitmapPtr bitmap;
    if (ImageEncoder::Compose(image_, selection, stack_, bitmap)) {
        ImageEncoder::CopyToClipboard(*bitmap, msgHwnd_);
    }
    EndSession();
}

void Application::Save() {
    if (!inSession_ || !overlay_) return;

    const HWND overlayHandle = overlay_->Handle();
    if (overlayHandle) ::ShowWindow(overlayHandle, SW_HIDE);

    std::wstring path;
    if (!ImageEncoder::AskSavePath(msgHwnd_, path)) {
        if (overlay_ && overlayHandle && ::IsWindow(overlayHandle)) {
            ::ShowWindow(overlayHandle, SW_SHOW);
        }
        return;
    }
    if (!inSession_ || !overlay_) return;

    const RECT selection = overlay_->Selection();
    ImageEncoder::BitmapPtr bitmap;
    if (ImageEncoder::Compose(image_, selection, stack_, bitmap)) {
        ImageEncoder::SavePng(*bitmap, path.c_str());
    }
    EndSession();
}

void Application::Quit() {
    overlay_.reset();
    stack_.Clear();
    image_.Release();
    inSession_ = false;
    hotkey_.Unregister(msgHwnd_);
    settings_.reset();
    if (msgHwnd_) ::DestroyWindow(msgHwnd_);
}

HWND Application::FindMessageWindow() {
    return ::FindWindowW(kMsgClassName, nullptr);
}

void Application::Notify(UINT message) {
    for (int attempt = 0; attempt < 6; ++attempt) {
        HWND target = FindMessageWindow();
        if (target) {
            ::PostMessageW(target, message, 0, 0);
            return;
        }
        ::Sleep(50);
    }
}

}  // namespace shot
