#pragma once

#include "annotation/AnnotationStack.h"
#include "annotation/IAnnotation.h"
#include "capture/ScreenCapture.h"
#include "common/Win32Types.h"
#include "overlay/SelectionController.h"
#include "overlay/Toolbar.h"

#include <windows.h>
#include <gdiplus.h>
#include <functional>
#include <string>
#include <vector>

namespace shot {

class OverlayWindow {
public:
    struct Callbacks {
        std::function<void()> onConfirm;
        std::function<void()> onSave;
        std::function<void()> onCancel;
    };

    OverlayWindow(HINSTANCE instance, CapturedImage& image, AnnotationStack& stack);
    ~OverlayWindow();

    OverlayWindow(const OverlayWindow&) = delete;
    OverlayWindow& operator=(const OverlayWindow&) = delete;

    bool Show();
    void Hide() noexcept;

    HWND Handle() const noexcept { return hwnd_; }
    RECT Selection() const noexcept { return selection_.Selection(); }
    bool HasSelection() const noexcept { return selection_.HasSelection(); }
    void SetCallbacks(Callbacks callbacks);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnPaint();
    void EnsureBackBuffer(int width, int height);
    void RequestRedraw();
    POINT ToVirtual(POINT client) const noexcept;

    void OnLButtonDown(POINT client);
    void OnMouseMove(POINT client);
    void OnLButtonUp(POINT client);
    void OnDblClick(POINT client);
    void OnKeyDown(WPARAM key);
    void OnChar(wchar_t ch);

    void HandleToolbarAction(ToolbarAction action);
    void CommitShape();
    void CommitText();
    void CancelText();
    void SetTool(Tool tool);

    void DrawPreview(Gdiplus::Graphics& g) const;
    void DrawSelectionChrome(Gdiplus::Graphics& g) const;
    void DrawTextInput(Gdiplus::Graphics& g) const;

    HINSTANCE instance_;
    HWND hwnd_ = nullptr;
    CapturedImage& image_;
    AnnotationStack& stack_;
    SelectionController selection_;
    Toolbar toolbar_;
    Callbacks callbacks_;

    POINT origin_{};
    RECT desktop_{};
    ScopedMemDC backDC_;
    ScopedBitmap backBitmap_;
    HGDIOBJ backOld_ = nullptr;
    int backWidth_ = 0;
    int backHeight_ = 0;

    Tool tool_ = Tool::Select;
    PenStyle style_;
    bool dragging_ = false;
    bool drawingShape_ = false;
    POINT shapeStart_{};
    POINT shapeEnd_{};
    std::vector<POINT> penPoints_;

    bool editingText_ = false;
    POINT textPos_{};
    std::wstring textBuffer_;
};

}  // namespace shot
