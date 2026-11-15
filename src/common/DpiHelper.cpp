#include "common/DpiHelper.h"

namespace shot::dpi {

void EnableAwareness() noexcept {
    HMODULE user32 = ::LoadLibraryW(L"user32.dll");
    if (user32) {
        using SetAwarenessFn = BOOL(WINAPI*)(void*);
        auto setAwareness = reinterpret_cast<SetAwarenessFn>(
            ::GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setAwareness) {
            void* perMonitorV2 = reinterpret_cast<void*>(static_cast<INT_PTR>(-4));
            if (setAwareness(perMonitorV2)) {
                ::FreeLibrary(user32);
                return;
            }
        }
        ::FreeLibrary(user32);
    }

    HMODULE shcore = ::LoadLibraryW(L"shcore.dll");
    if (shcore) {
        using SetProcessAwarenessFn = HRESULT(WINAPI*)(int);
        auto setProcessAwareness = reinterpret_cast<SetProcessAwarenessFn>(
            ::GetProcAddress(shcore, "SetProcessDpiAwareness"));
        if (setProcessAwareness && SUCCEEDED(setProcessAwareness(2))) {
            ::FreeLibrary(shcore);
            return;
        }
        ::FreeLibrary(shcore);
    }

    HMODULE fallback = ::LoadLibraryW(L"user32.dll");
    if (fallback) {
        using SetDpiAwareFn = BOOL(WINAPI*)();
        auto setDpiAware = reinterpret_cast<SetDpiAwareFn>(
            ::GetProcAddress(fallback, "SetProcessDPIAware"));
        if (setDpiAware) setDpiAware();
        ::FreeLibrary(fallback);
    }
}

RECT VirtualDesktop() noexcept {
    const int left = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int top = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int width = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int height = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
    RECT rc{ left, top, left + width, top + height };
    return rc;
}

POINT ClientToVirtual(POINT client, POINT origin) noexcept {
    POINT p{ client.x + origin.x, client.y + origin.y };
    return p;
}

POINT VirtualToClient(POINT virtualPoint, POINT origin) noexcept {
    POINT p{ virtualPoint.x - origin.x, virtualPoint.y - origin.y };
    return p;
}

}  // namespace shot::dpi
