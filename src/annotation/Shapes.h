#pragma once

#include "annotation/IAnnotation.h"

#include <functional>
#include <string>
#include <vector>

namespace shot {

using PixelSampler = std::function<COLORREF(int, int)>;

class RectShape : public IAnnotation {
public:
    RectShape(RECT rc, PenStyle style);
    void Draw(Gdiplus::Graphics& g) const override;
    RECT Bounds() const override;

private:
    RECT rc_;
    PenStyle style_;
};

class EllipseShape : public IAnnotation {
public:
    EllipseShape(RECT rc, PenStyle style);
    void Draw(Gdiplus::Graphics& g) const override;
    RECT Bounds() const override;

private:
    RECT rc_;
    PenStyle style_;
};

class ArrowShape : public IAnnotation {
public:
    ArrowShape(POINT from, POINT to, PenStyle style);
    void Draw(Gdiplus::Graphics& g) const override;
    RECT Bounds() const override;

private:
    POINT from_;
    POINT to_;
    PenStyle style_;
};

class PenShape : public IAnnotation {
public:
    PenShape(std::vector<POINT> points, PenStyle style);
    void Draw(Gdiplus::Graphics& g) const override;
    RECT Bounds() const override;

private:
    std::vector<POINT> points_;
    PenStyle style_;
};

class TextShape : public IAnnotation {
public:
    TextShape(POINT pos, std::wstring text, PenStyle style);
    void Draw(Gdiplus::Graphics& g) const override;
    RECT Bounds() const override;

private:
    POINT pos_;
    std::wstring text_;
    PenStyle style_;
};

class MosaicShape : public IAnnotation {
public:
    MosaicShape(RECT rc, const PixelSampler& sampler);
    void Draw(Gdiplus::Graphics& g) const override;
    RECT Bounds() const override;

private:
    struct Block {
        RECT rc;
        COLORREF color;
    };
    RECT rc_;
    std::vector<Block> blocks_;
};

}  // namespace shot
