
#include "Topology.hpp"

void* operator new(std::size_t, void* in) { return in; }

using namespace tp;

Vec3F TrigCache::gHitPos;

TrigCache::TrigCache() : mP1(0), mP2(0), mP3(0) {}

TrigCache::TrigCache(ualni v1, ualni v2, ualni v3) : mP1(v1), mP2(v2), mP3(v3) {}

TrigCache::TrigCache(const TrigCache& in) {
  mP1 = in.mP1;
  mP2 = in.mP2;
  mP3 = in.mP3;

  mEdgeP1P2 = in.mEdgeP1P2;
  mEdgeP1P3 = in.mEdgeP1P3;

  mOrigin = in.mOrigin;
  mNormal = in.mNormal;
}

void TrigCache::updateCache(const Buffer<Vec3F>& points) {
  mOrigin = points[mP1];
  mEdgeP1P2 = points[mP2] - points[mP1];
  mEdgeP1P3 = points[mP3] - points[mP1];
  mNormal = (points[mP2] - points[mP1]).cross(points[mP3] - points[mP1]).unitV();
}

bool TrigCache::castRay(const Ray& ray) const {
  static Vec3F h, s, q;
  static halnf a, f, u, v;
  static halnf t;

  if (ray.dir.dot(mNormal) > 0) {
    return false;
  }
  
  h = ray.dir.cross(mEdgeP1P3);
  a = mEdgeP1P2.dot(h);

  if (a > -EPSILON && a < EPSILON) {
    return false;
  }

  f = 1.f / a;
  s = ray.pos - mOrigin;
  u = f * s.dot(h);

  if (u < 0.0 || u > 1.0) {
    return false;
  }

  q = s.cross(mEdgeP1P2);
  v = f * ray.dir.dot(q);

  if (v < 0.f || u + v > 1.f) {
    return false;
  }

  t = f * mEdgeP1P3.dot(q);
  if (t > EPSILON) {
    gHitPos = ray.pos + ray.dir * t;
    return true;
  } else {
    return false;
  }
}

const Vec3F& TrigCache::getHitPos() { return gHitPos; }

const Vec3F& TrigCache::getNormal() const { return mNormal; }

void TopologyCache::updateCache() {
  TransformedNormals.clear();
  TransformedPoints.clear();
  TrigCaches.clear();

  if (!Source) {
    return;
  }

  TransformedPoints.reserve(Source->Points.size());
  for (auto idx : Range(TransformedPoints.size())) {
    TransformedPoints[idx] = Source->Basis.transform(Source->Points[idx]);
    TransformedPoints[idx] += Source->Origin;
  }

  TransformedNormals.reserve(Source->Normals.size());
  for (auto idx : Range(TransformedNormals.size())) {
    TransformedNormals[idx] = Source->Basis.transform(Source->Normals[idx]);
  }

  TrigCaches.reserve(Source->Indexes.size());
  for (auto idx : Range(TrigCaches.size())) {
    TrigCaches[idx].mP1 = Source->Indexes[idx].x;
    TrigCaches[idx].mP2 = Source->Indexes[idx].y;
    TrigCaches[idx].mP3 = Source->Indexes[idx].z;
    TrigCaches[idx].updateCache(TransformedPoints);
  }
}
