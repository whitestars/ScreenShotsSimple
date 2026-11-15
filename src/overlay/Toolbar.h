#pragma once

#include <windows.h>
#include <objbase.h>
#include <gdiplus.h>
#include <vector>

namespace shot {

enum class Tool { Select, Rect, Ellipse, Arrow, Pen, Text, Mosaic };

enum class ToolbarAction {
    None,
    Rect,
    Ellipse,
    Arrow,
    Pen,
    Text,
    Mosaic,
    Undo,
    Save,
    Confirm,
    Cancel
};

inline Tool ToolFromAction(ToolbarAction action) noexcept {
    switch (action) {
        case ToolbarAction::Rect: return Tool::Rect;
        case ToolbarAction::Ellipse: return Tool::Ellipse;
        case ToolbarAction::Arrow: return Tool::Arrow;
        case ToolbarAction::Pen: return Tool::Pen;
        case ToolbarAction::Text: return Tool::Text;
        case ToolbarAction::Mosaic: return Tool::Mosaic;
        default: return Tool::Select;
    }
}

inline ToolbarAction ActionFromTool(Tool tool) noexcept {
    switch (tool) {
        case Tool::Rect: return ToolbarAction::Rect;
        case Tool::Ellipse: return ToolbarAction::Ellipse;
        case Tool::Arrow: return ToolbarAction::Arrow;
        case Tool::Pen: return ToolbarAction::Pen;
        case Tool::Text: return ToolbarAction::Text;
        case Tool::Mosaic: return ToolbarAction::Mosaic;
        default: return ToolbarAction::None;
    }
}

class Toolbar {
public:
    void Layout(const RECT& selection, const RECT& desktop, bool hasSelection);
    ToolbarAction HitTest(POINT p) const noexcept;
    bool SetHover(ToolbarAction action) noexcept;
    void Draw(Gdiplus::Graphics& g, Tool current, bool canUndo) const;
    RECT Bounds() const noexcept { return frame_; }
    bool Visible() const noexcept { return visible_; }

private:
    struct Button {
        ToolbarAction action;
        const wchar_t* label;
        RECT rc;
    };

    static void DrawIcon(Gdiplus::Graphics& g, ToolbarAction action,
                         const RECT& rc, Gdiplus::ARGB color);
    void DrawTooltip(Gdiplus::Graphics& g) const;

    std::vector<Button> buttons_;
    RECT frame_{};
    RECT desktop_{};
    ToolbarAction hover_ = ToolbarAction::None;
    bool visible_ = false;
};

}  // namespace shot
