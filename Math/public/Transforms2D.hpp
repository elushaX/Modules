#pragma once

#include "Mat.hpp"
#include "Rect.hpp"

namespace tp {
  struct Transforms2D {
    Vec2F origin{};
    Mat2F mat{};

    [[nodiscard]] Vec2F toGlobal(const Vec2F &p) const;

    [[nodiscard]] Vec2F toLocal(const Vec2F &p) const;

    [[nodiscard]] RectF toGlobal(const RectF &in) const;

    [[nodiscard]] Transforms2D toGlobal(const Transforms2D &in);

    [[nodiscard]] Transforms2D inv();

    [[nodiscard]] Transforms2D relative(const Transforms2D &in);

    void operator+=(const Transforms2D &in);
  };
};