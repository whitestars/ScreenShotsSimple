#pragma once

#include "annotation/IAnnotation.h"

#include <vector>

namespace shot {

class AnnotationStack {
public:
    void Add(AnnotationPtr item);
    bool Undo() noexcept;
    void Clear() noexcept;
    bool Empty() const noexcept { return items_.empty(); }
    size_t Count() const noexcept { return items_.size(); }
    void DrawAll(Gdiplus::Graphics& g) const;
    RECT LastBounds() const noexcept;

private:
    std::vector<AnnotationPtr> items_;
};

}  // namespace shot
