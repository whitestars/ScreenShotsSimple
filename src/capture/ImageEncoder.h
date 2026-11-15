#pragma once

#include "annotation/AnnotationStack.h"
#include "capture/ScreenCapture.h"

#include <windows.h>
#include <gdiplus.h>
#include <memory>
#include <string>

namespace shot::ImageEncoder {

using BitmapPtr = std::unique_ptr<Gdiplus::Bitmap>;

bool Compose(const CapturedImage& image, const RECT& selection,
             const AnnotationStack& stack, BitmapPtr& out);

bool SavePng(Gdiplus::Bitmap& bitmap, const wchar_t* path);

bool CopyToClipboard(Gdiplus::Bitmap& bitmap, HWND owner);

bool AskSavePath(HWND owner, std::wstring& path);

}  // namespace shot::ImageEncoder
