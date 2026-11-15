#pragma once

#include <windows.h>

namespace shot {

class CapturedImage {
public:
    CapturedImage() = default;
    ~CapturedImage();
    CapturedImage(const CapturedImage&) = delete;
    CapturedImage& operator=(const CapturedImage&) = delete;

    bool Capture();
    void Release() noexcept;

    bool Valid() const noexcept { return bitmap_ != nullptr; }
    HBITMAP Bitmap() const noexcept { return bitmap_; }
    HDC Dc() const noexcept { return memDC_; }
    int Width() const noexcept { return width_; }
    int Height() const noexcept { return height_; }
    POINT Origin() const noexcept {
        POINT p{ originX_, originY_ };
        return p;
    }
    const unsigned char* Bits() const noexcept { return bits_; }
    int Stride() const noexcept { return stride_; }
    COLORREF Sample(int virtualX, int virtualY) const noexcept;

private:
    HBITMAP bitmap_ = nullptr;
    HDC memDC_ = nullptr;
    HGDIOBJ oldBitmap_ = nullptr;
    unsigned char* bits_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int originX_ = 0;
    int originY_ = 0;
    int stride_ = 0;
};

}  // namespace shot
