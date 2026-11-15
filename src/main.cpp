#include "app/Application.h"
#include "app/AutoStart.h"
#include "common/DpiHelper.h"
#include "common/Win32Types.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    shot::dpi::EnableAwareness();

    shot::ScopedHandle mutex(
        ::CreateMutexW(nullptr, TRUE, L"Local\\ScreenShotApp_SingleInstance"));
    const bool firstInstance = mutex && ::GetLastError() != ERROR_ALREADY_EXISTS;
    const auto command = shot::AutoStart::Parse(::GetCommandLineW());

    if (!firstInstance) {
        if (command == shot::AutoStart::Command::Quit) {
            shot::Application::Notify(shot::kAppMsgQuit);
        } else if (command == shot::AutoStart::Command::Screenshot) {
            shot::Application::Notify(shot::kAppMsgStart);
        } else if (command == shot::AutoStart::Command::Cancel) {
            shot::Application::Notify(shot::kAppMsgEndSession);
        } else if (command == shot::AutoStart::Command::None) {
            ::AllowSetForegroundWindow(ASFW_ANY);
            shot::Application::Notify(shot::kAppMsgSettings);
        }
        return 0;
    }

    if (command == shot::AutoStart::Command::Enable ||
        command == shot::AutoStart::Command::Disable ||
        command == shot::AutoStart::Command::Status) {
        return shot::AutoStart::Execute(command);
    }

    shot::GdiplusSession gdiplus;
    if (!gdiplus.Ok()) {
        shot::DebugLog(L"ScreenShotApp: GDI+ 初始化失败\n");
        return 1;
    }

    shot::Application app(instance);
    const bool showSettings =
        command != shot::AutoStart::Command::AutoStartLaunch;
    if (!app.Initialize(showSettings)) return 1;
    return app.Run();
}
