#include "annotation/Shapes.h"

#include "common/Win32Types.h"

#include <algorithm>
#include <cmath>

namespace shot {
namespace {

Gdiplus::Color ToColor(Gdiplus::ARGB argb) {
    return Gdiplus::Color(argb);
}

void ConfigurePen(Gdiplus::Pen& pen) {
    pen.SetStartCap(Gdiplus::LineCapRound);
    pen.SetEndCap(Gdiplus::LineCapRound);
    pen.SetLineJoin(Gdiplus::LineJoinRound);
}

RECT Expanded(RECT rc, float width) {
    const int pad = static_cast<int>(width) + 2;
    rc.left -= pad;
    rc.top -= pad;
    rc.right += pad;
    rc.bottom += pad;
    return rc;
}

}  // namespace

RectShape::RectShape(RECT rc, PenStyle style) : rc_(rc), style_(style) {}

void RectShape::Draw(Gdiplus::Graphics& g) const {
    const Gdiplus::Color color = ToColor(style_.color);
    Gdiplus::Pen pen(color, style_.width);
    ConfigurePen(pen);
    g.DrawRectangle(&pen,
                    static_cast<Gdiplus::REAL>(rc_.left),
                    static_cast<Gdiplus::REAL>(rc_.top),
                    static_cast<Gdiplus::REAL>(RectWidth(rc_)),
                    static_cast<Gdiplus::REAL>(RectHeight(rc_)));
}

RECT RectShape::Bounds() const {
    return Expanded(rc_, style_.width);
}

EllipseShape::EllipseShape(RECT rc, PenStyle style) : rc_(rc), style_(style) {}

void EllipseShape::Draw(Gdiplus::Graphics& g) const {
    const Gdiplus::Color color = ToColor(style_.color);
    Gdiplus::Pen pen(color, style_.width);
    ConfigurePen(pen);
    g.DrawEllipse(&pen,
                  static_cast<Gdiplus::REAL>(rc_.left),
                  static_cast<Gdiplus::REAL>(rc_.top),
                  static_cast<Gdiplus::REAL>(RectWidth(rc_)),
                  static_cast<Gdiplus::REAL>(RectHeight(rc_)));
}

RECT EllipseShape::Bounds() const {
    return Expanded(rc_, style_.width);
}

ArrowShape::ArrowShape(POINT from, POINT to, PenStyle style)
    : from_(from), to_(to), style_(style) {}

void ArrowShape::Draw(Gdiplus::Graphics& g) const {
    const double dx = static_cast<double>(to_.x) - from_.x;
    const double dy = static_cast<double>(to_.y) - from_.y;
    const double length = std::sqrt(dx * dx + dy * dy);
    if (length < 1.0) return;

    const Gdiplus::Color color = ToColor(style_.color);
    Gdiplus::Pen pen(color, style_.width);
    ConfigurePen(pen);
    g.DrawLine(&pen, static_cast<INT>(from_.x), static_cast<INT>(from_.y),
               static_cast<INT>(to_.x), static_cast<INT>(to_.y));

    const double angle = std::atan2(dy, dx);
    const double head = (std::min)(18.0, length * 0.35) + style_.width * 2.0;
    const double wing = 0.45;
    Gdiplus::PointF triangle[3];
    triangle[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(to_.x),
                                  static_cast<Gdiplus::REAL>(to_.y));
    triangle[1] = Gdiplus::PointF(
        static_cast<Gdiplus::REAL>(to_.x - head * std::cos(angle - wing)),
        static_cast<Gdiplus::REAL>(to_.y - head * std::sin(angle - wing)));
    triangle[2] = Gdiplus::PointF(
        static_cast<Gdiplus::REAL>(to_.x - head * std::cos(angle + wing)),
        static_cast<Gdiplus::REAL>(to_.y - head * std::sin(angle + wing)));

    Gdiplus::SolidBrush brush(color);
    g.FillPolygon(&brush, triangle, 3);
}

RECT ArrowShape::Bounds() const {
    return Expanded(NormalizeRect(from_, to_), style_.width);
}

PenShape::PenShape(std::vector<POINT> points, PenStyle style)
    : points_(std::move(points)), style_(style) {}

void PenShape::Draw(Gdiplus::Graphics& g) const {
    if (points_.empty()) return;

    const Gdiplus::Color color = ToColor(style_.color);
    Gdiplus::Pen pen(color, style_.width);
    ConfigurePen(pen);

    if (points_.size() == 1) {
        Gdiplus::SolidBrush brush(color);
        g.FillEllipse(&brush,
                      static_cast<Gdiplus::REAL>(points_[0].x - style_.width),
                      static_cast<Gdiplus::REAL>(points_[0].y - style_.width),
                      style_.width * 2.0f, style_.width * 2.0f);
        return;
    }

    std::vector<Gdiplus::Point> pts;
    pts.reserve(points_.size());
    for (const POINT& p : points_) pts.emplace_back(p.x, p.y);
    g.DrawLines(&pen, pts.data(), static_cast<INT>(pts.size()));
}

RECT PenShape::Bounds() const {
    if (points_.empty()) return MakeRect(0, 0, 0, 0);
    RECT rc = MakeRect(points_[0].x, points_[0].y, points_[0].x, points_[0].y);
    for (const POINT& p : points_) {
        if (p.x < rc.left) rc.left = p.x;
        if (p.y < rc.top) rc.top = p.y;
        if (p.x > rc.right) rc.right = p.x;
        if (p.y > rc.bottom) rc.bottom = p.y;
    }
    return Expanded(rc, style_.width);
}

TextShape::TextShape(POINT pos, std::wstring text, PenStyle style)
    : pos_(pos), text_(std::move(text)), style_(style) {}

void TextShape::Draw(Gdiplus::Graphics& g) const {
    if (text_.empty()) return;

    Gdiplus::SolidBrush brush(ToColor(style_.color));
    Gdiplus::Font font(L"Microsoft YaHei", 18.0f,
                       Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::PointF origin(static_cast<Gdiplus::REAL>(pos_.x),
                           static_cast<Gdiplus::REAL>(pos_.y));
    g.DrawString(text_.c_str(), static_cast<INT>(text_.size()), &font, origin, &brush);
}

RECT TextShape::Bounds() const {
    const int width = static_cast<int>(text_.size()) * 20 + 20;
    return MakeRect(pos_.x, pos_.y, pos_.x + width, pos_.y + 30);
}

MosaicShape::MosaicShape(RECT rc, const PixelSampler& sampler) : rc_(rc) {
    if (!sampler || RectEmpty(rc_)) return;

    const int width = RectWidth(rc_);
    const int height = RectHeight(rc_);
    const int block = (std::max)(
        8, static_cast<int>(std::sqrt(static_cast<double>(width) * height / 4000.0)));
    const int right = rc_.right;
    const int bottom = rc_.bottom;
    const int cols = (width + block - 1) / block;
    const int rows = (height + block - 1) / block;
    blocks_.reserve(static_cast<size_t>(cols) * rows);

    for (int y = rc_.top; y < bottom; y += block) {
        for (int x = rc_.left; x < right; x += block) {
            RECT cell{ x, y, (std::min)(x + block, right),
                       (std::min)(y + block, bottom) };
            const int stepX = (std::max)(1, RectWidth(cell) / 3);
            const int stepY = (std::max)(1, RectHeight(cell) / 3);
            long sumR = 0, sumG = 0, sumB = 0;
            int samples = 0;
            for (int sy = cell.top; sy < cell.bottom; sy += stepY) {
                for (int sx = cell.left; sx < cell.right; sx += stepX) {
                    const COLORREF c = sampler(sx, sy);
                    sumR += GetRValue(c);
                    sumG += GetGValue(c);
                    sumB += GetBValue(c);
                    ++samples;
                }
            }
            if (samples > 0) {
                blocks_.push_back(Block{
                    cell,
                    RGB(sumR / samples, sumG / samples, sumB / samples)
                });
            }
        }
    }
}

void MosaicShape::Draw(Gdiplus::Graphics& g) const {
    for (const Block& block : blocks_) {
        Gdiplus::Color color(255, GetRValue(block.color),
                             GetGValue(block.color), GetBValue(block.color));
        Gdiplus::SolidBrush brush(color);
        g.FillRectangle(&brush,
                        static_cast<Gdiplus::REAL>(block.rc.left),
                        static_cast<Gdiplus::REAL>(block.rc.top),
                        static_cast<Gdiplus::REAL>(RectWidth(block.rc)),
                        static_cast<Gdiplus::REAL>(RectHeight(block.rc)));
    }
}

RECT MosaicShape::Bounds() const {
    return rc_;
}

}  // namespace shot
