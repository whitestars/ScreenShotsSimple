#pragma once

#include <windows.h>
#include <string>

namespace shot::AutoStart {

enum class Command {
    None,
    Enable,
    Disable,
    Status,
    Quit,
    Screenshot,
    Cancel,
    AutoStartLaunch
};

Command Parse(const wchar_t* cmdLine);

int Execute(Command command);

bool SetEnabled(bool enable);

bool IsEnabled();

std::wstring ExePath();

}  // namespace shot::AutoStart
