#include "app/AutoStart.h"

#include <cwchar>

namespace shot::AutoStart {
namespace {

constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kValueName[] = L"ScreenShotApp";

bool HasFlag(const wchar_t* cmdLine, const wchar_t* flag) {
    return cmdLine && flag && std::wcsstr(cmdLine, flag) != nullptr;
}

}  // namespace

Command Parse(const wchar_t* cmdLine) {
    if (HasFlag(cmdLine, L"--autostart-enable")) return Command::Enable;
    if (HasFlag(cmdLine, L"--autostart-disable")) return Command::Disable;
    if (HasFlag(cmdLine, L"--autostart-status")) return Command::Status;
    if (HasFlag(cmdLine, L"--quit")) return Command::Quit;
    if (HasFlag(cmdLine, L"--snapshot")) return Command::Screenshot;
    if (HasFlag(cmdLine, L"--cancel")) return Command::Cancel;
    if (HasFlag(cmdLine, L"--autostart")) return Command::AutoStartLaunch;
    return Command::None;
}

std::wstring ExePath() {
    wchar_t path[MAX_PATH]{};
    ::GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::wstring(path);
}

bool SetEnabled(bool enable) {
    HKEY key = nullptr;
    LONG result = ::RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr,
                                    0, KEY_SET_VALUE | KEY_QUERY_VALUE,
                                    nullptr, &key, nullptr);
    if (result != ERROR_SUCCESS) return false;

    if (enable) {
        const std::wstring quoted = L"\"" + ExePath() + L"\" --autostart";
        result = ::RegSetValueExW(
            key, kValueName, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(quoted.c_str()),
            static_cast<DWORD>((quoted.size() + 1) * sizeof(wchar_t)));
    } else {
        result = ::RegDeleteValueW(key, kValueName);
        if (result == ERROR_FILE_NOT_FOUND) result = ERROR_SUCCESS;
    }

    ::RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

bool IsEnabled() {
    HKEY key = nullptr;
    if (::RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_QUERY_VALUE, &key)
        != ERROR_SUCCESS) {
        return false;
    }
    wchar_t buffer[1024]{};
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    const LONG result = ::RegQueryValueExW(key, kValueName, nullptr, &type,
                                           reinterpret_cast<BYTE*>(buffer), &size);
    ::RegCloseKey(key);
    return result == ERROR_SUCCESS && type == REG_SZ;
}

int Execute(Command command) {
    switch (command) {
        case Command::Enable: {
            const bool ok = SetEnabled(true);
            ::MessageBoxW(nullptr,
                          ok ? L"已开启开机自启动" : L"开启开机自启动失败",
                          L"ScreenShotApp", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        case Command::Disable: {
            const bool ok = SetEnabled(false);
            ::MessageBoxW(nullptr,
                          ok ? L"已关闭开机自启动" : L"关闭开机自启动失败",
                          L"ScreenShotApp", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        case Command::Status: {
            ::MessageBoxW(nullptr,
                          IsEnabled() ? L"开机自启动：已开启" : L"开机自启动：已关闭",
                          L"ScreenShotApp", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        default:
            return 1;
    }
}

}  // namespace shot::AutoStart
