#include "capture/ScreenCapture.h"

#include "common/DpiHelper.h"
#include "common/Win32Types.h"

namespace shot {

CapturedImage::~CapturedImage() {
    Release();
}

void CapturedImage::Release() noexcept {
    if (memDC_) {
        if (oldBitmap_) ::SelectObject(memDC_, oldBitmap_);
        ::DeleteDC(memDC_);
    }
    if (bitmap_) ::DeleteObject(bitmap_);
    memDC_ = nullptr;
    bitmap_ = nullptr;
    oldBitmap_ = nullptr;
    bits_ = nullptr;
    width_ = 0;
    height_ = 0;
    originX_ = 0;
    originY_ = 0;
    stride_ = 0;
}

bool CapturedImage::Capture() {
    Release();

    const RECT desktop = dpi::VirtualDesktop();
    width_ = RectWidth(desktop);
    height_ = RectHeight(desktop);
    originX_ = desktop.left;
    originY_ = desktop.top;
    if (width_ <= 0 || height_ <= 0) return false;

    ScopedScreenDC screenDC;
    if (!screenDC) {
        DebugLog(L"ScreenShotApp: 无法获取屏幕 DC\n");
        return false;
    }

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width_;
    info.bmiHeader.biHeight = -height_;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* pixelBits = nullptr;
    ScopedBitmap bitmap(::CreateDIBSection(screenDC.Get(), &info, DIB_RGB_COLORS,
                                           &pixelBits, nullptr, 0));
    if (!bitmap) {
        DebugLog(L"ScreenShotApp: 创建位图失败\n");
        return false;
    }

    ScopedMemDC memDC(::CreateCompatibleDC(screenDC.Get()));
    if (!memDC) {
        DebugLog(L"ScreenShotApp: 创建内存 DC 失败\n");
        return false;
    }

    HGDIOBJ old = ::SelectObject(memDC.Get(), bitmap.Get());
    if (!::BitBlt(memDC.Get(), 0, 0, width_, height_, screenDC.Get(),
                  originX_, originY_, SRCCOPY)) {
        ::SelectObject(memDC.Get(), old);
        DebugLog(L"ScreenShotApp: 抓屏失败\n");
        return false;
    }

    bitmap_ = bitmap.Release();
    memDC_ = memDC.Release();
    oldBitmap_ = old;
    bits_ = static_cast<unsigned char*>(pixelBits);
    stride_ = width_ * 4;
    return true;
}

COLORREF CapturedImage::Sample(int virtualX, int virtualY) const noexcept {
    const int x = virtualX - originX_;
    const int y = virtualY - originY_;
    if (!bits_ || x < 0 || y < 0 || x >= width_ || y >= height_) {
        return RGB(0, 0, 0);
    }
    const unsigned char* p = bits_ + static_cast<size_t>(y) * stride_ +
                             static_cast<size_t>(x) * 4;
    return RGB(p[2], p[1], p[0]);
}

}  // namespace shot
