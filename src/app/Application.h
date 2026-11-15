#pragma once

#include "annotation/AnnotationStack.h"
#include "app/HotkeyManager.h"
#include "app/SettingsWindow.h"
#include "capture/ScreenCapture.h"
#include "overlay/OverlayWindow.h"

#include <windows.h>
#include <memory>

namespace shot {

inline constexpr UINT kAppMsgStart = WM_APP + 1;
inline constexpr UINT kAppMsgQuit = WM_APP + 2;
inline constexpr UINT kAppMsgConfirm = WM_APP + 3;
inline constexpr UINT kAppMsgSave = WM_APP + 4;
inline constexpr UINT kAppMsgEndSession = WM_APP + 5;
inline constexpr UINT kAppMsgTray = WM_APP + 6;
inline constexpr UINT kAppMsgSettings = WM_APP + 7;

class Application {
public:
    explicit Application(HINSTANCE instance);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool Initialize(bool showSettings);
    int Run();

    static HWND FindMessageWindow();
    static void Notify(UINT message);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool CreateMessageWindow();
    bool AddTrayIcon();
    void ShowTrayNotice();
    void RefreshTrayTip();
    void RemoveTrayIcon() noexcept;
    void ShowTrayMenu();
    void ShowSettings();
    void CreateSettingsWindow(bool show);
    SettingsWindow::ApplyResult ApplySettings(HotkeyPreset preset, bool autoStart);

    void StartSession();
    void EndSession();
    void Confirm();
    void Save();
    void Quit();

    HINSTANCE instance_;
    HWND msgHwnd_ = nullptr;
    HotkeyManager hotkey_;
    HotkeyPreset preset_ = HotkeyPreset::CtrlAltA;
    CapturedImage image_;
    AnnotationStack stack_;
    std::unique_ptr<OverlayWindow> overlay_;
    std::unique_ptr<SettingsWindow> settings_;
    bool inSession_ = false;
    bool trayAdded_ = false;
};

}  // namespace shot
