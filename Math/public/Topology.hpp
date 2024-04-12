#pragma once

#include "Buffer.hpp"
#include "Camera.hpp"

namespace tp {

	class TrigCache {
		static Vec3F gHitPos;

	public:
		ualni mP1, mP2, mP3;
		Vec3F mEdgeP1P2, mEdgeP1P3;
		Vec3F mNormal;
		Vec3F mOrigin;

	public:
		TrigCache();
		TrigCache(const TrigCache& in);
		TrigCache(ualni v1, ualni v2, ualni v3);

	public:
		static const Vec3F& getHitPos();
		[[nodiscard]] const Vec3F& getNormal() const;

	public:
		void updateCache(const Buffer<Vec3F>& points);
		[[nodiscard]] bool castRay(const Ray& ray) const;
	};

	struct Topology {
		Vec3F Origin = { 0, 0, 0 };
		mat3f Basis = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

		Buffer<Vec3F> Points;
		Buffer<Vec3F> Normals;
		Buffer<Vec3<uhalni>> Indexes;
	};

	struct TopologyCache {
		const Topology* Source = nullptr;

		Buffer<Vec3F> TransformedPoints;
		Buffer<Vec3F> TransformedNormals;
		Buffer<TrigCache> TrigCaches;

		void updateCache();
	};
}
