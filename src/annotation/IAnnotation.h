#pragma once

#include <windows.h>
#include <objbase.h>
#include <gdiplus.h>
#include <memory>

namespace shot {

struct PenStyle {
    Gdiplus::ARGB color = 0xFFFF3B3B;
    float width = 2.0f;
};

class IAnnotation {
public:
    virtual ~IAnnotation() = default;
    virtual void Draw(Gdiplus::Graphics& g) const = 0;
    virtual RECT Bounds() const = 0;
};

using AnnotationPtr = std::unique_ptr<IAnnotation>;

}  // namespace shot
