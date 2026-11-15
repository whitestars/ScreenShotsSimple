#include "overlay/SelectionController.h"

#include "common/Win32Types.h"

namespace shot {

void SelectionController::Begin(POINT p, const RECT& bounds) {
    anchor_ = ClampPoint(p, bounds);
    previous_ = selection_;
    previousValid_ = has_;
    selection_ = MakeRect(anchor_.x, anchor_.y, anchor_.x, anchor_.y);
    dragging_ = true;
    created_ = false;
    has_ = false;
}

void SelectionController::Update(POINT p, const RECT& bounds) {
    if (!dragging_) return;
    selection_ = NormalizeRect(anchor_, ClampPoint(p, bounds));
    created_ = !RectEmpty(selection_);
}

void SelectionController::End() noexcept {
    dragging_ = false;
    if (created_ && !RectEmpty(selection_)) {
        has_ = true;
    } else {
        selection_ = previous_;
        has_ = previousValid_;
    }
    created_ = false;
    previousValid_ = false;
    previous_ = RECT{};
}

void SelectionController::Reset() noexcept {
    anchor_ = POINT{};
    selection_ = RECT{};
    previous_ = RECT{};
    previousValid_ = false;
    dragging_ = false;
    created_ = false;
    has_ = false;
}

}  // namespace shot
