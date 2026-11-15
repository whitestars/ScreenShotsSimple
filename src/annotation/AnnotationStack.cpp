#include "annotation/AnnotationStack.h"

#include "common/Win32Types.h"

namespace shot {

void AnnotationStack::Add(AnnotationPtr item) {
    if (item) items_.push_back(std::move(item));
}

bool AnnotationStack::Undo() noexcept {
    if (items_.empty()) return false;
    items_.pop_back();
    return true;
}

void AnnotationStack::Clear() noexcept {
    items_.clear();
}

void AnnotationStack::DrawAll(Gdiplus::Graphics& g) const {
    for (const auto& item : items_) {
        if (item) item->Draw(g);
    }
}

RECT AnnotationStack::LastBounds() const noexcept {
    if (items_.empty()) return MakeRect(0, 0, 0, 0);
    return items_.back()->Bounds();
}

}  // namespace shot
