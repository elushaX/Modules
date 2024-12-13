#include "Transforms2D.hpp"

using namespace tp;

Vec2F Transforms2D::toGlobal(const Vec2F &p) const {
  return mat.toGlobal(p) + origin;
}

Vec2F Transforms2D::toLocal(const Vec2F &p) const {
  auto tmp = p - origin;
  return mat.toLocal(tmp);
}

RectF Transforms2D::toGlobal(const RectF &in) const {
  auto p1 = toGlobal(in.p1());
  auto p2 = toGlobal(in.p3());
  auto pMin = Vec2F{std::min(p1.x, p2.x), std::min(p1.y, p2.y)};
  auto pMax = Vec2F{std::max(p1.x, p2.x), std::max(p1.y, p2.y)};
  return {pMin, pMax - pMin};
}

Transforms2D Transforms2D::toGlobal(const Transforms2D &in) {
  Transforms2D out = *this;
  out += in;
  return out;
}

Transforms2D Transforms2D::inv() {
  Transforms2D out;
  out.mat = mat.inv();
  out.origin = mat.toLocal({-origin.x, -origin.y});
  return out;
}

Transforms2D Transforms2D::relative(const Transforms2D &in) {
  return inv().toGlobal(in);
}

void Transforms2D::operator+=(const Transforms2D &in) {
  origin = toGlobal(in.origin);
  mat = mat.toGlobal(in.mat);
}
