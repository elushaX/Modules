
// #include "NewPlacement.hpp"

#include "RayTracer.hpp"
#include "UnitTest++/UnitTest++.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace tp;

void writeImage(const RayTracer::RenderBuffer& output) {
	// Save the data to a PNG file
	struct urgb {
		uint1 r, g, b, a;
	};

	Buffer2D<urgb> converted;
	converted.reserve(output.size());

	for (Index i = 0; i < output.size().x; i++) {
		for (Index j = 0; j < output.size().y; j++) {
			converted.get({ i, j }).r = uint1(output.get({ i, j }).r * 255);
			converted.get({ i, j }).g = uint1(output.get({ i, j }).g * 255);
			converted.get({ i, j }).b = uint1(output.get({ i, j }).b * 255);
			converted.get({ i, j }).a = uint1(output.get({ i, j }).a * 255);
		}
	}

	stbi_write_png("output.png", converted.size().x, converted.size().y, 4, converted.getBuff(), converted.size().x * 4);
}

bool compareCols(const RGBA& l, const RGBA& r) {
	auto small = 0.0001f;
	if ((l.r - r.r) > small) {
		return false;
	}
	if ((l.g - r.g) > small) {
		return false;
	}
	if ((l.b - r.b) > small) {
		return false;
	}
	if ((l.a - r.a) > small) {
		return false;
	}
	return true;
}

SUITE(RayTracer) {
	TEST(Basic) {
		using namespace tp;

		Scene scene;

		scene.mCamera.lookAtPoint({ 0, 0, 0 }, { 2, 2, 2 }, { 0, 0, 1 });
		scene.mCamera.setFOV(3.14 / 4);

		scene.mLights.append({ { 0, 0, 1.1f }, 1.f, 0.3f });

		scene.mObjects.append(Object());
		auto& object = scene.mObjects.last();

		object.mTopology.Points = {
			{ 1.000000, 0.000000, 0.000000 },
			{ 0.000000, 1.000000, 0.000000 },
			{ 0.000000, 0.000000, 1.000000 },
		};

		object.mTopology.Normals = {
			{ 1.000000, 1.000000, 1.000000 },
			{ 1.000000, 1.000000, 1.000000 },
			{ 1.000000, 1.000000, 1.000000 },
		};

		object.mTopology.Indexes = {
			{ 0, 1, 2 },
		};

		object.mCache.Source = &object.mTopology;
		object.mCache.updateCache();

		RayTracer::RenderSettings settings = {
			0,
			0,
			1,
			{ 10, 10 },
		};

		RayTracer::OutputBuffers output;
		// output.reserve(RayTracer::RenderBuffer::Index2D(settings.size.x, settings.size.y));

		RayTracer rt;
		rt.render(scene, output, settings);

		CHECK(compareCols(output.color.get({ 6, 4 }), RGBA{ 0.560100f, 0.560100f, 0.560100f, 1.000000f }));
		CHECK(compareCols(output.color.get({ 6, 5 }), RGBA{ 0.353739f, 0.353739f, 0.353739f, 1.000000f }));
		CHECK(compareCols(output.color.get({ 6, 6 }), RGBA{ 0.242577f, 0.242577f, 0.242577f, 1.000000f }));
		CHECK(compareCols(output.color.get({ 6, 7 }), RGBA{ 0.176313f, 0.176313f, 0.176313f, 1.000000f }));
		CHECK(compareCols(output.color.get({ 6, 8 }), RGBA{ 0.000000f, 0.000000f, 0.000000f, 0.000000f }));

		if (0) {
			writeImage(output.color);
			for (Index i = 0; i < output.color.size().x; i++) {
				for (Index j = 0; j < output.color.size().y; j++) {
					auto tmp = output.color.get({ i, j });
					printf(
						"TEST(compareCols(output.get({%i, %i}), RGBA{ %ff, %ff, %ff, %ff }));\n", i, j, tmp.r, tmp.g, tmp.b, tmp.a
					);
				}
			}
		}
	}
}

int main(int argc, char* argv[]) {
	return UnitTest::RunAllTests();
}
