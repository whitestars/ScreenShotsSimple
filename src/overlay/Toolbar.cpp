#include "overlay/Toolbar.h"

#include "common/Win32Types.h"

#include <algorithm>
#include <cwchar>

namespace shot {
namespace {

struct ButtonDef {
    ToolbarAction action;
    const wchar_t* label;
};

constexpr ButtonDef kButtons[] = {
    { ToolbarAction::Rect, L"矩形" },
    { ToolbarAction::Ellipse, L"椭圆" },
    { ToolbarAction::Arrow, L"箭头" },
    { ToolbarAction::Pen, L"画笔" },
    { ToolbarAction::Text, L"文字" },
    { ToolbarAction::Mosaic, L"马赛克" },
    { ToolbarAction::Undo, L"撤销" },
    { ToolbarAction::Save, L"保存" },
    { ToolbarAction::Confirm, L"完成" },
    { ToolbarAction::Cancel, L"取消" },
};

constexpr int kButtonWidth = 34;
constexpr int kButtonHeight = 30;
constexpr int kGap = 3;
constexpr int kPadding = 6;

}  // namespace

void Toolbar::Layout(const RECT& selection, const RECT& desktop, bool hasSelection) {
    buttons_.clear();
    desktop_ = desktop;
    visible_ = hasSelection && !RectEmpty(selection);
    if (!visible_) {
        frame_ = RECT{};
        hover_ = ToolbarAction::None;
        return;
    }

    constexpr int count = static_cast<int>(sizeof(kButtons) / sizeof(kButtons[0]));
    const int totalWidth = kPadding * 2 + count * kButtonWidth + (count - 1) * kGap;
    const int totalHeight = kPadding * 2 + kButtonHeight;

    int left = selection.left + (RectWidth(selection) - totalWidth) / 2;
    const int minLeft = desktop.left + 4;
    const int maxLeft = desktop.right - totalWidth - 4;
    left = (std::max)(minLeft, (std::min)(left, maxLeft));

    int top = selection.bottom + 10;
    if (top + totalHeight > desktop.bottom) {
        top = selection.top - totalHeight - 10;
    }
    if (top < desktop.top) top = desktop.top + 4;

    frame_ = MakeRect(left, top, left + totalWidth, top + totalHeight);

    int x = left + kPadding;
    const int y = top + kPadding;
    buttons_.reserve(count);
    for (const ButtonDef& def : kButtons) {
        buttons_.push_back(Button{
            def.action, def.label,
            MakeRect(x, y, x + kButtonWidth, y + kButtonHeight)
        });
        x += kButtonWidth + kGap;
    }
}

ToolbarAction Toolbar::HitTest(POINT p) const noexcept {
    if (!visible_) return ToolbarAction::None;
    for (const Button& button : buttons_) {
        if (::PtInRect(&button.rc, p)) return button.action;
    }
    return ToolbarAction::None;
}

bool Toolbar::SetHover(ToolbarAction action) noexcept {
    if (!visible_) action = ToolbarAction::None;
    if (hover_ == action) return false;
    hover_ = action;
    return true;
}

void Toolbar::DrawIcon(Gdiplus::Graphics& g, ToolbarAction action,
                       const RECT& rc, Gdiplus::ARGB argb) {
    Gdiplus::Color iconColor(argb);
    Gdiplus::Pen pen(iconColor, 1.6f);
    pen.SetStartCap(Gdiplus::LineCapRound);
    pen.SetEndCap(Gdiplus::LineCapRound);
    pen.SetLineJoin(Gdiplus::LineJoinRound);
    Gdiplus::SolidBrush brush(iconColor);

    const Gdiplus::REAL cx = static_cast<Gdiplus::REAL>(rc.left) + 17.0f;
    const Gdiplus::REAL cy = static_cast<Gdiplus::REAL>(rc.top) + 15.0f;

    switch (action) {
        case ToolbarAction::Rect:
            g.DrawRectangle(&pen, cx - 8.0f, cy - 6.0f, 16.0f, 12.0f);
            break;
        case ToolbarAction::Ellipse:
            g.DrawEllipse(&pen, cx - 8.0f, cy - 6.0f, 16.0f, 12.0f);
            break;
        case ToolbarAction::Arrow: {
            g.DrawLine(&pen, cx - 8.0f, cy + 6.0f, cx + 7.0f, cy - 6.0f);
            Gdiplus::PointF head[3] = {
                Gdiplus::PointF(cx + 8.0f, cy - 7.0f),
                Gdiplus::PointF(cx + 1.0f, cy - 6.0f),
                Gdiplus::PointF(cx + 6.0f, cy + 1.0f)
            };
            g.FillPolygon(&brush, head, 3);
            break;
        }
        case ToolbarAction::Pen: {
            Gdiplus::PointF stroke[5] = {
                Gdiplus::PointF(cx - 8.0f, cy + 5.0f),
                Gdiplus::PointF(cx - 4.0f, cy - 4.0f),
                Gdiplus::PointF(cx, cy + 3.0f),
                Gdiplus::PointF(cx + 4.0f, cy - 5.0f),
                Gdiplus::PointF(cx + 8.0f, cy - 1.0f)
            };
            g.DrawLines(&pen, stroke, 5);
            break;
        }
        case ToolbarAction::Text:
            g.DrawLine(&pen, cx - 7.0f, cy - 6.0f, cx + 7.0f, cy - 6.0f);
            g.DrawLine(&pen, cx, cy - 6.0f, cx, cy + 7.0f);
            g.DrawLine(&pen, cx - 4.0f, cy + 7.0f, cx + 4.0f, cy + 7.0f);
            break;
        case ToolbarAction::Mosaic:
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 3; ++col) {
                    const Gdiplus::REAL x = cx - 8.0f + col * 5.5f;
                    const Gdiplus::REAL y = cy - 7.5f + row * 5.5f;
                    if ((row + col) % 2 == 0) {
                        g.FillRectangle(&brush, x, y, 4.5f, 4.5f);
                    } else {
                        g.DrawRectangle(&pen, x, y, 4.5f, 4.5f);
                    }
                }
            }
            break;
        case ToolbarAction::Undo: {
            g.DrawLine(&pen, cx + 7.0f, cy + 3.0f, cx - 3.0f, cy + 3.0f);
            g.DrawLine(&pen, cx - 3.0f, cy + 3.0f, cx - 3.0f, cy - 4.0f);
            Gdiplus::PointF head[3] = {
                Gdiplus::PointF(cx - 8.0f, cy - 3.0f),
                Gdiplus::PointF(cx + 2.0f, cy - 3.0f),
                Gdiplus::PointF(cx - 3.0f, cy - 9.0f)
            };
            g.FillPolygon(&brush, head, 3);
            break;
        }
        case ToolbarAction::Save:
            g.DrawRectangle(&pen, cx - 7.0f, cy - 7.0f, 14.0f, 14.0f);
            g.FillRectangle(&brush, cx - 4.0f, cy - 7.0f, 8.0f, 5.0f);
            g.DrawRectangle(&pen, cx - 4.0f, cy + 1.0f, 8.0f, 7.0f);
            break;
        case ToolbarAction::Confirm: {
            Gdiplus::PointF check[3] = {
                Gdiplus::PointF(cx - 7.0f, cy),
                Gdiplus::PointF(cx - 2.0f, cy + 5.0f),
                Gdiplus::PointF(cx + 8.0f, cy - 6.0f)
            };
            g.DrawLines(&pen, check, 3);
            break;
        }
        case ToolbarAction::Cancel:
            g.DrawLine(&pen, cx - 6.0f, cy - 6.0f, cx + 6.0f, cy + 6.0f);
            g.DrawLine(&pen, cx + 6.0f, cy - 6.0f, cx - 6.0f, cy + 6.0f);
            break;
        default:
            break;
    }
}

void Toolbar::DrawTooltip(Gdiplus::Graphics& g) const {
    if (!visible_ || hover_ == ToolbarAction::None) return;

    const wchar_t* label = nullptr;
    RECT hoverRect{};
    for (const Button& button : buttons_) {
        if (button.action == hover_) {
            label = button.label;
            hoverRect = button.rc;
            break;
        }
    }
    if (!label) return;

    const Gdiplus::REAL width =
        static_cast<Gdiplus::REAL>(std::wcslen(label)) * 11.0f + 14.0f;
    const Gdiplus::REAL height = 20.0f;
    Gdiplus::REAL x = static_cast<Gdiplus::REAL>(hoverRect.left) +
        (static_cast<Gdiplus::REAL>(RectWidth(hoverRect)) - width) / 2.0f;
    Gdiplus::REAL y = static_cast<Gdiplus::REAL>(frame_.top) - height - 4.0f;
    if (y < static_cast<Gdiplus::REAL>(desktop_.top)) {
        y = static_cast<Gdiplus::REAL>(frame_.bottom) + 4.0f;
    }

    Gdiplus::SolidBrush background(Gdiplus::Color(235, 18, 20, 28));
    g.FillRectangle(&background, x, y, width, height);
    Gdiplus::Font font(L"Microsoft YaHei", 10.0f,
                       Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush foreground(Gdiplus::Color(255, 235, 240, 250));
    Gdiplus::PointF origin(x + 7.0f, y + 2.0f);
    g.DrawString(label, -1, &font, origin, &foreground);
}

void Toolbar::Draw(Gdiplus::Graphics& g, Tool current, bool canUndo) const {
    if (!visible_) return;

    Gdiplus::SolidBrush background(Gdiplus::Color(238, 28, 30, 38));
    g.FillRectangle(&background,
                    static_cast<Gdiplus::REAL>(frame_.left),
                    static_cast<Gdiplus::REAL>(frame_.top),
                    static_cast<Gdiplus::REAL>(RectWidth(frame_)),
                    static_cast<Gdiplus::REAL>(RectHeight(frame_)));

    Gdiplus::Pen border(Gdiplus::Color(110, 255, 255, 255), 1.0f);
    g.DrawRectangle(&border,
                    static_cast<Gdiplus::REAL>(frame_.left),
                    static_cast<Gdiplus::REAL>(frame_.top),
                    static_cast<Gdiplus::REAL>(RectWidth(frame_)),
                    static_cast<Gdiplus::REAL>(RectHeight(frame_)));

    const ToolbarAction active = ActionFromTool(current);
    for (const Button& button : buttons_) {
        if (button.action == hover_) {
            Gdiplus::SolidBrush hover(Gdiplus::Color(35, 255, 255, 255));
            g.FillRectangle(&hover,
                            static_cast<Gdiplus::REAL>(button.rc.left),
                            static_cast<Gdiplus::REAL>(button.rc.top),
                            static_cast<Gdiplus::REAL>(RectWidth(button.rc)),
                            static_cast<Gdiplus::REAL>(RectHeight(button.rc)));
        }
        if (button.action == active) {
            Gdiplus::SolidBrush highlight(Gdiplus::Color(70, 80, 150, 240));
            g.FillRectangle(&highlight,
                            static_cast<Gdiplus::REAL>(button.rc.left),
                            static_cast<Gdiplus::REAL>(button.rc.top),
                            static_cast<Gdiplus::REAL>(RectWidth(button.rc)),
                            static_cast<Gdiplus::REAL>(RectHeight(button.rc)));
        }

        Gdiplus::ARGB argb = 0xFFE8EDF6;
        if (button.action == ToolbarAction::Undo && !canUndo) argb = 0xFF606878;
        if (button.action == ToolbarAction::Confirm) argb = 0xFF7CE0A0;
        if (button.action == ToolbarAction::Cancel) argb = 0xFFFF8A8A;
        DrawIcon(g, button.action, button.rc, argb);
    }

    DrawTooltip(g);
}

}  // namespace shot
