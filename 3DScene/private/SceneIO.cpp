
#include "Scene.hpp"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

#include "obj/OBJ_Loader.h"

#include <filesystem>

bool tp::Scene::load(const std::string& objetsPath) {
	using namespace tp;

	objl::Loader Loader;

	if (!Loader.LoadFile(objetsPath)) {
		std::cout << "Failed to Load File. May have failed to find it or it was not an .obj file.\n";
		return false;
	}

	for (auto& curMesh : Loader.LoadedMeshes) {
		mObjects.append(Object());

		auto object = &mObjects.last();

		for (auto& vertex : curMesh.Vertices) {
			// printf("{ %f, %f, %f }, \n", vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
			object->mTopology.Points.append(Vec3F{ vertex.Position.X, vertex.Position.Y, vertex.Position.Z });
			object->mTopology.Normals.append(Vec3F{ vertex.Normal.X, vertex.Normal.Y, vertex.Normal.Z });
		}

		for (int j = 0; j < curMesh.Indices.size(); j += 3) {
			auto idx1 = (uhalni) curMesh.Indices[j];
			auto idx2 = (uhalni) curMesh.Indices[j + 1];
			auto idx3 = (uhalni) curMesh.Indices[j + 2];
			// printf("{ %i, %i, %i },\n", idx1, idx2, idx3);
			object->mTopology.Indexes.append(Vec3<uhalni>{ idx1, idx2, idx3 });
		}

		if (object->mTopology.Normals.size() != object->mTopology.Points.size()) {
			printf("Logic error loading normals\n");
		}

		object->mCache.Source = &object->mTopology;
		object->mCache.updateCache();
	}

	return mObjects.size();
}