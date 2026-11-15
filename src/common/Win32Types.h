#pragma once

#include <windows.h>
#include <objbase.h>
#include <gdiplus.h>
#include <algorithm>
#include <cstddef>

namespace shot {

inline void DebugLog(const wchar_t* text) noexcept {
    if (text) ::OutputDebugStringW(text);
}

class ScopedHandle {
public:
    ScopedHandle() noexcept = default;
    explicit ScopedHandle(HANDLE h) noexcept : h_(h) {}
    ~ScopedHandle() { Reset(); }
    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;
    ScopedHandle(ScopedHandle&& other) noexcept : h_(other.Release()) {}
    ScopedHandle& operator=(ScopedHandle&& other) noexcept {
        if (this != &other) Reset(other.Release());
        return *this;
    }
    void Reset(HANDLE h = nullptr) noexcept {
        if (h_ && h_ != INVALID_HANDLE_VALUE) ::CloseHandle(h_);
        h_ = h;
    }
    HANDLE Get() const noexcept { return h_; }
    HANDLE Release() noexcept {
        HANDLE tmp = h_;
        h_ = nullptr;
        return tmp;
    }
    explicit operator bool() const noexcept { return h_ != nullptr && h_ != INVALID_HANDLE_VALUE; }

private:
    HANDLE h_ = nullptr;
};

class ScopedMemDC {
public:
    ScopedMemDC() noexcept = default;
    explicit ScopedMemDC(HDC dc) noexcept : dc_(dc) {}
    ~ScopedMemDC() { Reset(); }
    ScopedMemDC(const ScopedMemDC&) = delete;
    ScopedMemDC& operator=(const ScopedMemDC&) = delete;
    void Reset(HDC dc = nullptr) noexcept {
        if (dc_) ::DeleteDC(dc_);
        dc_ = dc;
    }
    HDC Get() const noexcept { return dc_; }
    HDC Release() noexcept {
        HDC tmp = dc_;
        dc_ = nullptr;
        return tmp;
    }
    explicit operator bool() const noexcept { return dc_ != nullptr; }

private:
    HDC dc_ = nullptr;
};

class ScopedScreenDC {
public:
    explicit ScopedScreenDC(HWND hwnd = nullptr) noexcept
        : hwnd_(hwnd), dc_(::GetDC(hwnd)) {}
    ~ScopedScreenDC() {
        if (dc_) ::ReleaseDC(hwnd_, dc_);
    }
    ScopedScreenDC(const ScopedScreenDC&) = delete;
    ScopedScreenDC& operator=(const ScopedScreenDC&) = delete;
    HDC Get() const noexcept { return dc_; }
    explicit operator bool() const noexcept { return dc_ != nullptr; }

private:
    HWND hwnd_;
    HDC dc_;
};

class ScopedBitmap {
public:
    ScopedBitmap() noexcept = default;
    explicit ScopedBitmap(HBITMAP bmp) noexcept : bmp_(bmp) {}
    ~ScopedBitmap() { Reset(); }
    ScopedBitmap(const ScopedBitmap&) = delete;
    ScopedBitmap& operator=(const ScopedBitmap&) = delete;
    void Reset(HBITMAP bmp = nullptr) noexcept {
        if (bmp_) ::DeleteObject(bmp_);
        bmp_ = bmp;
    }
    HBITMAP Get() const noexcept { return bmp_; }
    HBITMAP Release() noexcept {
        HBITMAP tmp = bmp_;
        bmp_ = nullptr;
        return tmp;
    }
    explicit operator bool() const noexcept { return bmp_ != nullptr; }

private:
    HBITMAP bmp_ = nullptr;
};

class ScopedSelect {
public:
    ScopedSelect(HDC dc, HGDIOBJ obj) noexcept
        : dc_(dc), old_(dc ? ::SelectObject(dc, obj) : nullptr) {}
    ~ScopedSelect() {
        if (dc_ && old_) ::SelectObject(dc_, old_);
    }
    ScopedSelect(const ScopedSelect&) = delete;
    ScopedSelect& operator=(const ScopedSelect&) = delete;

private:
    HDC dc_;
    HGDIOBJ old_;
};

class GdiplusSession {
public:
    GdiplusSession() noexcept {
        Gdiplus::GdiplusStartupInput input;
        ok_ = (Gdiplus::GdiplusStartup(&token_, &input, nullptr) == Gdiplus::Ok);
    }
    ~GdiplusSession() {
        if (ok_) Gdiplus::GdiplusShutdown(token_);
    }
    GdiplusSession(const GdiplusSession&) = delete;
    GdiplusSession& operator=(const GdiplusSession&) = delete;
    bool Ok() const noexcept { return ok_; }

private:
    ULONG_PTR token_ = 0;
    bool ok_ = false;
};

class ClipboardGuard {
public:
    explicit ClipboardGuard(HWND owner) noexcept
        : opened_(::OpenClipboard(owner) != FALSE) {}
    ~ClipboardGuard() {
        if (opened_) ::CloseClipboard();
    }
    ClipboardGuard(const ClipboardGuard&) = delete;
    ClipboardGuard& operator=(const ClipboardGuard&) = delete;
    bool Ok() const noexcept { return opened_; }

private:
    bool opened_;
};

inline RECT MakeRect(int left, int top, int right, int bottom) noexcept {
    RECT rc{ left, top, right, bottom };
    return rc;
}

inline RECT NormalizeRect(POINT a, POINT b) noexcept {
    RECT rc{
        (std::min)(a.x, b.x), (std::min)(a.y, b.y),
        (std::max)(a.x, b.x), (std::max)(a.y, b.y)
    };
    return rc;
}

inline int RectWidth(const RECT& rc) noexcept { return rc.right - rc.left; }
inline int RectHeight(const RECT& rc) noexcept { return rc.bottom - rc.top; }
inline bool RectEmpty(const RECT& rc) noexcept {
    return rc.right <= rc.left || rc.bottom <= rc.top;
}

inline POINT ClampPoint(POINT p, const RECT& bounds) noexcept {
    if (p.x < bounds.left) p.x = bounds.left;
    if (p.x > bounds.right) p.x = bounds.right;
    if (p.y < bounds.top) p.y = bounds.top;
    if (p.y > bounds.bottom) p.y = bounds.bottom;
    return p;
}

inline RECT IntersectRect(const RECT& a, const RECT& b) noexcept {
    RECT rc{
        (std::max)(a.left, b.left), (std::max)(a.top, b.top),
        (std::min)(a.right, b.right), (std::min)(a.bottom, b.bottom)
    };
    return rc;
}

inline RECT UnionRect(const RECT& a, const RECT& b) noexcept {
    RECT rc{
        (std::min)(a.left, b.left), (std::min)(a.top, b.top),
        (std::max)(a.right, b.right), (std::max)(a.bottom, b.bottom)
    };
    return rc;
}

}  // namespace shot
