#include "capture/ImageEncoder.h"

#include "common/Win32Types.h"

#include <commdlg.h>
#include <objbase.h>

#include <cstdio>
#include <cstring>
#include <cwchar>
#include <vector>

namespace shot::ImageEncoder {
namespace {

bool GetPngClsid(CLSID* clsid) {
    UINT count = 0;
    UINT size = 0;
    if (Gdiplus::GetImageEncodersSize(&count, &size) != Gdiplus::Ok || size == 0) {
        return false;
    }
    std::vector<unsigned char> buffer(size);
    auto* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
    if (Gdiplus::GetImageEncoders(count, size, codecs) != Gdiplus::Ok) return false;
    for (UINT i = 0; i < count; ++i) {
        if (codecs[i].MimeType && std::wcscmp(codecs[i].MimeType, L"image/png") == 0) {
            *clsid = codecs[i].Clsid;
            return true;
        }
    }
    return false;
}

bool PutPngOnClipboard(Gdiplus::Bitmap& bitmap) noexcept {
    CLSID clsid{};
    if (!GetPngClsid(&clsid)) return false;

    IStream* stream = nullptr;
    if (FAILED(::CreateStreamOnHGlobal(nullptr, TRUE, &stream)) || !stream) return false;

    bool copied = false;
    if (bitmap.Save(stream, &clsid, nullptr) == Gdiplus::Ok) {
        HGLOBAL source = nullptr;
        if (SUCCEEDED(::GetHGlobalFromStream(stream, &source)) && source) {
            const SIZE_T bytes = ::GlobalSize(source);
            if (bytes > 0) {
                HGLOBAL target = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
                if (target) {
                    void* src = ::GlobalLock(source);
                    void* dst = ::GlobalLock(target);
                    if (src && dst) std::memcpy(dst, src, bytes);
                    if (src) ::GlobalUnlock(source);
                    if (dst) ::GlobalUnlock(target);

                    const UINT format = ::RegisterClipboardFormatW(L"PNG");
                    if (format != 0 && src && dst && ::SetClipboardData(format, target)) {
                        copied = true;
                    } else {
                        ::GlobalFree(target);
                    }
                }
            }
        }
    }
    stream->Release();
    return copied;
}

}  // namespace

bool Compose(const CapturedImage& image, const RECT& selection,
             const AnnotationStack& stack, BitmapPtr& out) {
    out.reset();
    if (!image.Valid() || !image.Bits() || RectEmpty(selection)) return false;

    const POINT origin = image.Origin();
    const RECT desktop = MakeRect(origin.x, origin.y,
                                  origin.x + image.Width(), origin.y + image.Height());
    const RECT sel = IntersectRect(selection, desktop);
    if (RectEmpty(sel)) return false;

    const int width = RectWidth(sel);
    const int height = RectHeight(sel);
    auto bitmap = std::make_unique<Gdiplus::Bitmap>(width, height,
                                                    PixelFormat32bppARGB);
    if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok) return false;

    Gdiplus::Rect lockRect(0, 0, width, height);
    Gdiplus::BitmapData data{};
    if (bitmap->LockBits(&lockRect, Gdiplus::ImageLockModeWrite,
                         PixelFormat32bppARGB, &data) != Gdiplus::Ok) {
        return false;
    }

    const int srcX = sel.left - origin.x;
    const int srcY = sel.top - origin.y;
    const int dstStride = data.Stride >= 0 ? data.Stride : -data.Stride;
    auto* scan0 = static_cast<unsigned char*>(data.Scan0);
    for (int y = 0; y < height; ++y) {
        const unsigned char* src = image.Bits() +
            static_cast<size_t>(srcY + y) * image.Stride() +
            static_cast<size_t>(srcX) * 4;
        unsigned char* dst = scan0 + static_cast<size_t>(y) * dstStride;
        std::memcpy(dst, src, static_cast<size_t>(width) * 4);
        for (int x = 0; x < width; ++x) dst[x * 4 + 3] = 255;
    }
    bitmap->UnlockBits(&data);

    {
        Gdiplus::Graphics g(bitmap.get());
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
        g.TranslateTransform(-static_cast<Gdiplus::REAL>(sel.left),
                             -static_cast<Gdiplus::REAL>(sel.top));
        stack.DrawAll(g);
    }

    out = std::move(bitmap);
    return true;
}

bool SavePng(Gdiplus::Bitmap& bitmap, const wchar_t* path) {
    CLSID clsid{};
    if (!path || !GetPngClsid(&clsid)) return false;
    return bitmap.Save(path, &clsid, nullptr) == Gdiplus::Ok;
}

bool CopyToClipboard(Gdiplus::Bitmap& bitmap, HWND owner) {
    HBITMAP handle = nullptr;
    if (bitmap.GetHBITMAP(Gdiplus::Color(0xFF000000), &handle) != Gdiplus::Ok || !handle) {
        DebugLog(L"ScreenShotApp: 生成剪贴板位图失败\n");
        return false;
    }

    ClipboardGuard clipboard(owner);
    if (!clipboard.Ok() || !::EmptyClipboard()) {
        ::DeleteObject(handle);
        DebugLog(L"ScreenShotApp: 打开剪贴板失败\n");
        return false;
    }

    const bool ok = ::SetClipboardData(CF_BITMAP, handle) != nullptr;
    PutPngOnClipboard(bitmap);
    if (!ok) {
        ::DeleteObject(handle);
        DebugLog(L"ScreenShotApp: 写入剪贴板失败\n");
    }
    return ok;
}

bool AskSavePath(HWND owner, std::wstring& path) {
    wchar_t file[MAX_PATH]{};
    SYSTEMTIME now{};
    ::GetLocalTime(&now);
    std::swprintf(file, MAX_PATH, L"ScreenShot_%04d%02d%02d_%02d%02d%02d.png",
                  now.wYear, now.wMonth, now.wDay,
                  now.wHour, now.wMinute, now.wSecond);

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = L"PNG 图片\0*.png\0\0";
    dialog.lpstrFile = file;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrInitialDir = nullptr;  // 交给系统：首次默认位置，之后记忆上次目录
    dialog.lpstrTitle = L"保存截图";
    dialog.lpstrDefExt = L"png";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!::GetSaveFileNameW(&dialog)) return false;
    path = file;
    return true;
}

}  // namespace shot::ImageEncoder
