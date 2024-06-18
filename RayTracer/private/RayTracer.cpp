
#include "MathCommon.hpp"

#include "ConnectionCommon.hpp"
#include "Module.hpp"
#include "Ray.hpp"
#include "RayTracer.hpp"
#include "TypeInfo.hpp"

#include <cstdio>

/*
 if (1) {
 const auto& points = castData.obj->mCache.TransformedPoints;
 const auto& normals = castData.obj->mCache.TransformedNormals;
 const auto trig = castData.trig;

const auto& n1 = normals[trig->mP1];
const auto& n2 = normals[trig->mP2];
const auto& n3 = normals[trig->mP3];

auto v0 = points[trig->mP1];
auto v1 = points[trig->mP2];
auto v2 = points[trig->mP3];

// Calculate barycentric coordinates
Vec3F barycentric;

// Calculate the area of the triangle
auto areaABC = (halnf) (v1 - v0).cross(v2 - v0).length();
auto areaPBC = (halnf) (v1 - castData.hitPos).cross(v2 - castData.hitPos).length();
auto areaPCA = (halnf) (v2 - castData.hitPos).cross(v0 - castData.hitPos).length();

// Calculate the barycentric coordinates
barycentric.x = areaPBC / areaABC;
barycentric.y = areaPCA / areaABC;
barycentric.z = 1.0f - barycentric.x - barycentric.y;

// Interpolate the normal using barycentric coordinates
normal = n1 * barycentric.x + n2 * barycentric.y + n3 * barycentric.z;
}
*/

using namespace tp;

void RayTracer::castRay(const Ray& ray, RayCastData& out, alnf farVal) {
	out.hit = false;
	out.obj = nullptr;

	farVal *= farVal;

	for (auto obj : mScene->mObjects) {
		for (auto trig : obj->mCache.TrigCaches) {
			if (trig->castRay(ray)) {
				// printf("Hit\n");

				auto dist = (trig->getHitPos() - ray.pos).length2();

				if (farVal > dist && dist > EPSILON) {
					out.trig = &trig.data();
					out.hitPos = trig->getHitPos();
					out.obj = &obj.data();
					out.hit = true;

					farVal = dist;
				}
			}
		}
	}
}

void RayTracer::cycle(const RayCastData& castData, LightData& out, uhalni depth) {
	if (depth) {
		depth--;

		Vec3F normal = castData.trig->getNormal();
		normal.normalize();

		const auto delta1 = castData.trig->mEdgeP1P2.unitV();
		const auto delta2 = normal.cross(delta1);

		for (auto idx : Range(mSettings.spray)) {
			RayCastData materialCastData;
			LightData lightData;

			auto d1 = ((halnf) randomFloat() - 0.5f) * 2;
			auto d2 = ((halnf) randomFloat() - 0.5f) * 2;

			auto sprayNormal = (normal + delta1 * d1 + delta2 * d2).normalize();

			castRay({ sprayNormal, castData.hitPos }, materialCastData, mScene->mCamera.getFar());
			if (materialCastData.hit) {
				cycle(materialCastData, lightData, depth);
				out.intensity += lightData.intensity * 0.2;
			}
		}
	}

	// cast for light
	for (auto light : mScene->mLights) {
		RayCastData lightCastData;
		auto dir = light->pos - castData.hitPos;
		auto length = (halnf) dir.length();

		Ray lightRay = { dir.unitV(), castData.hitPos };

		if (lightRay.dir.dot(castData.trig->mNormal) < 0) {
			continue;
		}
		castRay(lightRay, lightCastData, length);

		if (lightCastData.hit) {
			continue;
		}

		out.intensity += light->intensity / (length * length);
	}
}

void RayTracer::render(const Scene& scene, OutputBuffers& out, const RenderSettings& settings) {
	out.color.reserve({ settings.size.x, settings.size.y });
	out.normals.reserve({ settings.size.x, settings.size.y });
	out.depth.reserve({ settings.size.x, settings.size.y });
	out.albedo.reserve({ settings.size.x, settings.size.y });

	mScene = &scene;
	mSettings = settings;

	auto pos = mScene->mCamera.getPos();
	auto camera = mScene->mCamera;

	const auto planeLeftTop = camera.project({ -1, -1 });
	const auto planeRightTop = camera.project({ 1, -1 });
	const auto planeRightBottom = camera.project({ 1, 1 });

	const auto up = (planeRightBottom - planeRightTop);
	const auto right = planeRightTop - planeLeftTop;

	RayCastData castData;

	Ray ray = { { 0, 0, 0 }, pos };

	Vec3F iterPoint = { 0, 0, 0 };
	Vec3F deltaX = right / halnf(mSettings.size.x);
	Vec3F deltaY = up / halnf(mSettings.size.y);

	ualni maxIterations = mSettings.size.x * mSettings.size.y;
	ualni currIter = 0;

	halnf maxDepth = 0;
	halnf minDepth = mScene->mCamera.getFar() * mSettings.multisampling;

	auto accumulateColor = [](RGBA& col, const RGBA& in) {
		col.r += in.r;
		col.g += in.g;
		col.b += in.b;
		col.a = 1;
	};

	auto divideColor = [](RGBA& col, const halnf num) {
		col.r /= num;
		col.g /= num;
		col.b /= num;
	};

	// clear colors
	for (ualni i = 0; i < mSettings.size.x; i++) {
		for (ualni j = 0; j < mSettings.size.y; j++) {
			out.color.set({ i, j }, 0.f);
			out.normals.set({ i, j }, 0.f);
			out.depth.set({ i, j }, 0.f);
			out.albedo.set({ i, j }, 0.f);
		}
	}

	for (ualni i = 0; i < mSettings.size.x; i++) {
		for (ualni j = 0; j < mSettings.size.y; j++) {
			for (auto sample = 0; sample < mSettings.multisampling; sample++) {
				auto randX = randomFloat();
				auto randY = randomFloat();

				iterPoint = planeLeftTop + ((deltaX * (halnf) (i + randX)) + (deltaY * (halnf) (j + randY)));
				ray.dir = (iterPoint - pos).unitV();

				castRay(ray, castData, mScene->mCamera.getFar());

				halni albedoColor = abs(hash((ualni) castData.obj));
				halnf albedoColorR = float((albedoColor & 0x00000011) % 155) + 100;
				halnf albedoColorG = float((albedoColor & 0x00001100) % 155) + 100;
				halnf albedoColorB = float((albedoColor & 0x00110000) % 155) + 100;

				out.albedo.set({ i, j }, { albedoColorR, albedoColorG, albedoColorB, 1.f });

				if (castData.hit) {
					LightData lightData;
					cycle(castData, lightData, mSettings.depth);

					const auto normal = castData.trig->getNormal();
					const auto depth = (halnf) (castData.hitPos - ray.pos).length();

					lightData.intensity = clamp(lightData.intensity, 0.f, 1.f);
					RGBA col = { lightData.intensity, lightData.intensity, lightData.intensity, 1.f };

					accumulateColor(out.color.get({ i, j }), col);
					accumulateColor(out.normals.get({ i, j }), { normal.x * 0.5f + 0.5f, normal.y * 0.5f + 0.5f, normal.z * 0.5f + 0.5f, 1.f });
					accumulateColor(out.depth.get({ i, j }), { depth, depth, depth, 1.f });

				} else {
					// out.color.set({ i, j }, 0.f);
					// out.normals.set({ i, j }, 0.f);
					// out.depth.set({ i, j }, 0.f);
				}

				// auto tmp = buff.get({i, j});
				// printf(" %f, %f, %f, %f, ", tmp.r, tmp.g, tmp.b, tmp.a);
			}

			mProgress.percentage = (halnf) currIter / (halnf) maxIterations;
			currIter++;
		}
	}

	for (auto i = 0; i < mSettings.size.x * mSettings.size.y; i++) {
		divideColor(out.color.getBuff()[i], (halnf) mSettings.multisampling);
		divideColor(out.normals.getBuff()[i], (halnf) mSettings.multisampling);
		divideColor(out.depth.getBuff()[i], (halnf) mSettings.multisampling);
	}

	for (auto i = 0; i < mSettings.size.x * mSettings.size.y; i++) {
		if (!out.depth.getBuff()[i].a) {
			continue;
		}
		const auto depth = out.depth.getBuff()[i].r;
		if (maxDepth < depth) {
			maxDepth = depth;
		}
		if (minDepth > depth) {
			minDepth = depth;
		}
	}

	for (auto i = 0; i < mSettings.size.x * mSettings.size.y; i++) {
		auto& col = out.depth.getBuff()[i];
		if (col.a == 1.f) {
			col.r = (col.r - minDepth) / (maxDepth - minDepth);
			col.g = col.r;
			col.b = col.r;
		}
	}
}
