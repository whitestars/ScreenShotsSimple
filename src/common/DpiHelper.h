#pragma once

#include <windows.h>

namespace shot::dpi {

void EnableAwareness() noexcept;

RECT VirtualDesktop() noexcept;

POINT ClientToVirtual(POINT client, POINT origin) noexcept;

POINT VirtualToClient(POINT virtualPoint, POINT origin) noexcept;

}  // namespace shot::dpi
