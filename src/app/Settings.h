#pragma once

#include "app/HotkeyManager.h"

namespace shot::Settings {

HotkeyPreset LoadHotkey();

bool SaveHotkey(HotkeyPreset preset);

}  // namespace shot::Settings
