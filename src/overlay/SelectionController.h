#pragma once

#include <windows.h>

namespace shot {

class SelectionController {
public:
    void Begin(POINT p, const RECT& bounds);
    void Update(POINT p, const RECT& bounds);
    void End() noexcept;

    bool IsDragging() const noexcept { return dragging_; }
    bool HasSelection() const noexcept { return has_ || dragging_; }
    RECT Selection() const noexcept {
        return (dragging_ && !created_) ? previous_ : selection_;
    }
    void Reset() noexcept;

private:
    POINT anchor_{};
    RECT selection_{};
    RECT previous_{};
    bool previousValid_ = false;
    bool dragging_ = false;
    bool created_ = false;
    bool has_ = false;
};

}  // namespace shot
