#include "overlay/OverlayWindow.h"

#include "annotation/Shapes.h"
#include "common/DpiHelper.h"

#include <windowsx.h>

#include <cstdlib>
#include <cwchar>
#include <memory>
#include <utility>

namespace shot {
namespace {

constexpr wchar_t kOverlayClassName[] = L"ScreenShotApp_Overlay";

POINT PointFromLParam(LPARAM lParam) noexcept {
    POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    return p;
}

void SetLineCaps(Gdiplus::Pen& pen) {
    pen.SetStartCap(Gdiplus::LineCapRound);
    pen.SetEndCap(Gdiplus::LineCapRound);
    pen.SetLineJoin(Gdiplus::LineJoinRound);
}

}  // namespace

OverlayWindow::OverlayWindow(HINSTANCE instance, CapturedImage& image,
                             AnnotationStack& stack)
    : instance_(instance), image_(image), stack_(stack) {}

OverlayWindow::~OverlayWindow() {
    Hide();
}

void OverlayWindow::SetCallbacks(Callbacks callbacks) {
    callbacks_ = std::move(callbacks);
}

bool OverlayWindow::Show() {
    if (hwnd_) return true;
    if (!image_.Valid()) return false;

    origin_ = image_.Origin();
    desktop_ = dpi::VirtualDesktop();

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = &OverlayWindow::WndProc;
    wc.hInstance = instance_;
    wc.hCursor = ::LoadCursorW(nullptr, IDC_CROSS);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kOverlayClassName;
    if (!::RegisterClassExW(&wc) && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        DebugLog(L"ScreenShotApp: 注册遮罩窗口类失败\n");
        return false;
    }

    hwnd_ = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        kOverlayClassName, L"",
        WS_POPUP | WS_VISIBLE,
        desktop_.left, desktop_.top, RectWidth(desktop_), RectHeight(desktop_),
        nullptr, nullptr, instance_, this);
    if (!hwnd_) {
        DebugLog(L"ScreenShotApp: 创建遮罩窗口失败\n");
        return false;
    }

    ::SetForegroundWindow(hwnd_);
    ::SetFocus(hwnd_);
    RequestRedraw();
    return true;
}

void OverlayWindow::Hide() noexcept {
    if (hwnd_ && ::GetCapture() == hwnd_) ::ReleaseCapture();
    if (hwnd_) {
        ::DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }

    if (backDC_.Get() && backOld_) ::SelectObject(backDC_.Get(), backOld_);
    backDC_.Reset();
    backBitmap_.Reset();
    backOld_ = nullptr;
    backWidth_ = 0;
    backHeight_ = 0;

    selection_.Reset();
    toolbar_.Layout(RECT{}, desktop_, false);
    tool_ = Tool::Select;
    dragging_ = false;
    drawingShape_ = false;
    editingText_ = false;
    textBuffer_.clear();
    penPoints_.clear();
}

LRESULT CALLBACK OverlayWindow::WndProc(HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                            reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }
    auto* self = reinterpret_cast<OverlayWindow*>(
        ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) return self->OnMessage(hwnd, msg, wParam, lParam);
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT OverlayWindow::OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND:
            return TRUE;
        case WM_PAINT:
            OnPaint();
            return 0;
        case WM_LBUTTONDOWN:
            OnLButtonDown(PointFromLParam(lParam));
            return 0;
        case WM_MOUSEMOVE:
            OnMouseMove(PointFromLParam(lParam));
            return 0;
        case WM_LBUTTONUP:
            OnLButtonUp(PointFromLParam(lParam));
            return 0;
        case WM_LBUTTONDBLCLK:
            OnDblClick(PointFromLParam(lParam));
            return 0;
        case WM_KEYDOWN:
            OnKeyDown(wParam);
            return 0;
        case WM_CHAR:
            OnChar(static_cast<wchar_t>(wParam));
            return 0;
        case WM_RBUTTONDOWN:
            if (callbacks_.onCancel) callbacks_.onCancel();
            return 0;
        case WM_NCDESTROY:
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            return 0;
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void OverlayWindow::EnsureBackBuffer(int width, int height) {
    if (backDC_ && backWidth_ == width && backHeight_ == height) return;

    if (backDC_.Get()) {
        if (backOld_) ::SelectObject(backDC_.Get(), backOld_);
        backDC_.Reset();
    }
    backBitmap_.Reset();
    backOld_ = nullptr;
    backWidth_ = 0;
    backHeight_ = 0;

    ScopedScreenDC screen;
    if (!screen) return;
    ScopedBitmap bitmap(::CreateCompatibleBitmap(screen.Get(), width, height));
    ScopedMemDC dc(::CreateCompatibleDC(screen.Get()));
    if (!bitmap || !dc) return;

    backOld_ = ::SelectObject(dc.Get(), bitmap.Get());
    backBitmap_.Reset(bitmap.Release());
    backDC_.Reset(dc.Release());
    backWidth_ = width;
    backHeight_ = height;
}

void OverlayWindow::OnPaint() {
    PAINTSTRUCT ps{};
    HDC hdc = ::BeginPaint(hwnd_, &ps);
    if (!hdc) return;

    const int width = RectWidth(desktop_);
    const int height = RectHeight(desktop_);
    EnsureBackBuffer(width, height);
    HDC mem = backDC_.Get();
    if (!mem || !image_.Valid()) {
        ::EndPaint(hwnd_, &ps);
        return;
    }

    ::BitBlt(mem, 0, 0, width, height, image_.Dc(), 0, 0, SRCCOPY);

    {
        Gdiplus::Graphics g(mem);
        Gdiplus::SolidBrush dim(Gdiplus::Color(150, 12, 14, 20));
        g.FillRectangle(&dim, 0.0f, 0.0f,
                        static_cast<Gdiplus::REAL>(width),
                        static_cast<Gdiplus::REAL>(height));
    }

    const RECT sel = selection_.Selection();
    if (selection_.HasSelection() && !RectEmpty(sel)) {
        const int x = sel.left - origin_.x;
        const int y = sel.top - origin_.y;
        ::BitBlt(mem, x, y, RectWidth(sel), RectHeight(sel),
                 image_.Dc(), x, y, SRCCOPY);
    }

    {
        Gdiplus::Graphics g(mem);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
        g.TranslateTransform(-static_cast<Gdiplus::REAL>(origin_.x),
                             -static_cast<Gdiplus::REAL>(origin_.y));
        stack_.DrawAll(g);
        DrawPreview(g);
        DrawSelectionChrome(g);
        toolbar_.Draw(g, tool_, !stack_.Empty());
        DrawTextInput(g);
    }

    ::BitBlt(hdc, 0, 0, width, height, mem, 0, 0, SRCCOPY);
    ::EndPaint(hwnd_, &ps);
}

void OverlayWindow::RequestRedraw() {
    toolbar_.Layout(selection_.Selection(), desktop_,
                    selection_.HasSelection() && !selection_.IsDragging());
    if (hwnd_) ::InvalidateRect(hwnd_, nullptr, FALSE);
}

POINT OverlayWindow::ToVirtual(POINT client) const noexcept {
    return dpi::ClientToVirtual(client, origin_);
}

void OverlayWindow::OnLButtonDown(POINT client) {
    const POINT p = ToVirtual(client);
    toolbar_.Layout(selection_.Selection(), desktop_,
                    selection_.HasSelection() && !selection_.IsDragging());
    const ToolbarAction action = toolbar_.HitTest(p);
    if (action != ToolbarAction::None) {
        HandleToolbarAction(action);
        return;
    }

    if (editingText_) CommitText();

    if (tool_ == Tool::Text) {
        editingText_ = true;
        textPos_ = p;
        textBuffer_.clear();
        RequestRedraw();
        return;
    }

    ::SetCapture(hwnd_);
    if (tool_ != Tool::Select) {
        drawingShape_ = true;
        shapeStart_ = p;
        shapeEnd_ = p;
        penPoints_.clear();
        if (tool_ == Tool::Pen) penPoints_.push_back(p);
        RequestRedraw();
        return;
    }

    selection_.Begin(p, desktop_);
    dragging_ = true;
    RequestRedraw();
}

void OverlayWindow::OnMouseMove(POINT client) {
    const POINT p = ToVirtual(client);
    if (drawingShape_) {
        toolbar_.SetHover(ToolbarAction::None);
        shapeEnd_ = p;
        if (tool_ == Tool::Pen) {
            if (penPoints_.empty() ||
                std::abs(p.x - penPoints_.back().x) > 1 ||
                std::abs(p.y - penPoints_.back().y) > 1) {
                penPoints_.push_back(p);
            }
        }
        RequestRedraw();
        return;
    }
    if (dragging_) {
        toolbar_.SetHover(ToolbarAction::None);
        selection_.Update(p, desktop_);
        RequestRedraw();
        return;
    }
    if (toolbar_.SetHover(toolbar_.HitTest(p))) RequestRedraw();
}

void OverlayWindow::OnLButtonUp(POINT client) {
    if (::GetCapture() == hwnd_) ::ReleaseCapture();
    const POINT p = ToVirtual(client);

    if (drawingShape_) {
        shapeEnd_ = p;
        if (tool_ == Tool::Pen && !penPoints_.empty()) penPoints_.push_back(p);
        CommitShape();
        drawingShape_ = false;
        penPoints_.clear();
        RequestRedraw();
        return;
    }
    if (dragging_) {
        selection_.Update(p, desktop_);
        selection_.End();
        dragging_ = false;
        RequestRedraw();
    }
}

void OverlayWindow::OnDblClick(POINT client) {
    const POINT p = ToVirtual(client);
    const RECT sel = selection_.Selection();
    if (selection_.HasSelection() && ::PtInRect(&sel, p)) {
        if (callbacks_.onConfirm) callbacks_.onConfirm();
    }
}

void OverlayWindow::OnKeyDown(WPARAM key) {
    const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;

    if (editingText_) {
        if (key == VK_RETURN) {
            CommitText();
        } else if (key == VK_ESCAPE) {
            CancelText();
        } else if (key == VK_BACK) {
            if (!textBuffer_.empty()) textBuffer_.pop_back();
            RequestRedraw();
        }
        return;
    }

    if (key == VK_ESCAPE) {
        if (editingText_) {
            CancelText();
        } else if (callbacks_.onCancel) {
            callbacks_.onCancel();
        }
        return;
    }
    if (key == VK_RETURN) {
        if (selection_.HasSelection() && callbacks_.onConfirm) callbacks_.onConfirm();
        return;
    }
    if (ctrl && key == 'Z') {
        stack_.Undo();
        RequestRedraw();
        return;
    }
    if (ctrl && key == 'S') {
        if (selection_.HasSelection() && callbacks_.onSave) callbacks_.onSave();
        return;
    }
    if (key >= '1' && key <= '6') {
        static const Tool kTools[] = {
            Tool::Rect, Tool::Ellipse, Tool::Arrow,
            Tool::Pen, Tool::Text, Tool::Mosaic
        };
        SetTool(kTools[key - '1']);
    }
}

void OverlayWindow::OnChar(wchar_t ch) {
    if (!editingText_ || ch < 0x20) return;
    textBuffer_.push_back(ch);
    RequestRedraw();
}

void OverlayWindow::HandleToolbarAction(ToolbarAction action) {
    switch (action) {
        case ToolbarAction::Undo:
            stack_.Undo();
            RequestRedraw();
            return;
        case ToolbarAction::Save:
            if (selection_.HasSelection() && callbacks_.onSave) callbacks_.onSave();
            return;
        case ToolbarAction::Confirm:
            if (selection_.HasSelection() && callbacks_.onConfirm) callbacks_.onConfirm();
            return;
        case ToolbarAction::Cancel:
            if (callbacks_.onCancel) callbacks_.onCancel();
            return;
        default:
            break;
    }
    SetTool(ToolFromAction(action));
}

void OverlayWindow::CommitShape() {
    const RECT rc = NormalizeRect(shapeStart_, shapeEnd_);
    PixelSampler sampler = [this](int x, int y) { return image_.Sample(x, y); };
    AnnotationPtr item;

    switch (tool_) {
        case Tool::Rect:
            if (!RectEmpty(rc)) item = std::make_unique<RectShape>(rc, style_);
            break;
        case Tool::Ellipse:
            if (!RectEmpty(rc)) item = std::make_unique<EllipseShape>(rc, style_);
            break;
        case Tool::Arrow:
            item = std::make_unique<ArrowShape>(shapeStart_, shapeEnd_, style_);
            break;
        case Tool::Pen:
            if (!penPoints_.empty()) item = std::make_unique<PenShape>(penPoints_, style_);
            break;
        case Tool::Mosaic:
            if (!RectEmpty(rc)) item = std::make_unique<MosaicShape>(rc, sampler);
            break;
        default:
            break;
    }

    if (item) stack_.Add(std::move(item));
    RequestRedraw();
}

void OverlayWindow::CommitText() {
    if (!editingText_) return;
    if (!textBuffer_.empty()) {
        stack_.Add(std::make_unique<TextShape>(textPos_, textBuffer_, style_));
    }
    editingText_ = false;
    textBuffer_.clear();
    RequestRedraw();
}

void OverlayWindow::CancelText() {
    if (!editingText_) return;
    editingText_ = false;
    textBuffer_.clear();
    RequestRedraw();
}

void OverlayWindow::SetTool(Tool tool) {
    if (editingText_ && tool != Tool::Text) {
        if (!textBuffer_.empty()) {
            stack_.Add(std::make_unique<TextShape>(textPos_, textBuffer_, style_));
        }
        editingText_ = false;
        textBuffer_.clear();
    }
    tool_ = tool;
    RequestRedraw();
}

void OverlayWindow::DrawPreview(Gdiplus::Graphics& g) const {
    if (!drawingShape_) return;

    Gdiplus::Pen pen(Gdiplus::Color(style_.color), style_.width);
    SetLineCaps(pen);
    const RECT rc = NormalizeRect(shapeStart_, shapeEnd_);
    const Gdiplus::REAL x = static_cast<Gdiplus::REAL>(rc.left);
    const Gdiplus::REAL y = static_cast<Gdiplus::REAL>(rc.top);
    const Gdiplus::REAL w = static_cast<Gdiplus::REAL>(RectWidth(rc));
    const Gdiplus::REAL h = static_cast<Gdiplus::REAL>(RectHeight(rc));

    switch (tool_) {
        case Tool::Rect:
            if (!RectEmpty(rc)) g.DrawRectangle(&pen, x, y, w, h);
            break;
        case Tool::Ellipse:
            if (!RectEmpty(rc)) g.DrawEllipse(&pen, x, y, w, h);
            break;
        case Tool::Arrow:
            g.DrawLine(&pen, static_cast<INT>(shapeStart_.x),
                       static_cast<INT>(shapeStart_.y),
                       static_cast<INT>(shapeEnd_.x),
                       static_cast<INT>(shapeEnd_.y));
            break;
        case Tool::Pen: {
            if (penPoints_.size() < 2) break;
            std::vector<Gdiplus::Point> pts;
            pts.reserve(penPoints_.size());
            for (const POINT& p : penPoints_) pts.emplace_back(p.x, p.y);
            g.DrawLines(&pen, pts.data(), static_cast<INT>(pts.size()));
            break;
        }
        case Tool::Mosaic:
            pen.SetDashStyle(Gdiplus::DashStyleDash);
            if (!RectEmpty(rc)) g.DrawRectangle(&pen, x, y, w, h);
            break;
        default:
            break;
    }
}

void OverlayWindow::DrawSelectionChrome(Gdiplus::Graphics& g) const {
    if (!selection_.HasSelection()) return;
    const RECT sel = selection_.Selection();
    if (RectEmpty(sel)) return;

    Gdiplus::Pen border(Gdiplus::Color(255, 80, 160, 255), 1.5f);
    g.DrawRectangle(&border,
                    static_cast<Gdiplus::REAL>(sel.left),
                    static_cast<Gdiplus::REAL>(sel.top),
                    static_cast<Gdiplus::REAL>(RectWidth(sel)),
                    static_cast<Gdiplus::REAL>(RectHeight(sel)));

    Gdiplus::SolidBrush handle(Gdiplus::Color(255, 255, 255, 255));
    const int corners[4][2] = {
        { sel.left, sel.top }, { sel.right, sel.top },
        { sel.left, sel.bottom }, { sel.right, sel.bottom }
    };
    for (const auto& corner : corners) {
        g.FillRectangle(&handle,
                        static_cast<Gdiplus::REAL>(corner[0] - 3),
                        static_cast<Gdiplus::REAL>(corner[1] - 3),
                        6.0f, 6.0f);
    }

    wchar_t sizeText[64]{};
    std::swprintf(sizeText, 64, L"%d x %d", RectWidth(sel), RectHeight(sel));
    Gdiplus::Font font(L"Consolas", 12.0f,
                       Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush background(Gdiplus::Color(210, 20, 22, 30));
    Gdiplus::SolidBrush foreground(Gdiplus::Color(255, 235, 240, 250));

    const int tx = sel.left;
    int ty = sel.top - 24;
    if (ty < desktop_.top + 2) ty = sel.top + 6;
    g.FillRectangle(&background, static_cast<Gdiplus::REAL>(tx),
                    static_cast<Gdiplus::REAL>(ty), 92.0f, 20.0f);
    g.DrawString(sizeText, -1, &font,
                 Gdiplus::PointF(static_cast<Gdiplus::REAL>(tx + 6),
                                static_cast<Gdiplus::REAL>(ty + 2)),
                 &foreground);
}

void OverlayWindow::DrawTextInput(Gdiplus::Graphics& g) const {
    if (!editingText_) return;
    const std::wstring display = textBuffer_ + L"|";
    Gdiplus::Font font(L"Microsoft YaHei", 18.0f,
                       Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush brush(Gdiplus::Color(style_.color));
    g.DrawString(display.c_str(), static_cast<INT>(display.size()), &font,
                 Gdiplus::PointF(static_cast<Gdiplus::REAL>(textPos_.x),
                                static_cast<Gdiplus::REAL>(textPos_.y)),
                 &brush);
}

}  // namespace shot
