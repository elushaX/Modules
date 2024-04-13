#include "Scene.hpp"

#include "obj/OBJ_Loader.h"
#include <filesystem>

bool loadMeshes(tp::Scene& scene, const std::string& objetsPath) {
	using namespace tp;

	objl::Loader Loader;

	if (!Loader.LoadFile(objetsPath.c_str())) {
		std::cout << "Failed to Load File. May have failed to find it or it was not an .obj file.\n";
		return false;
	}

	for (auto& curMesh : Loader.LoadedMeshes) {
		scene.mObjects.append(Object());

		auto object = &scene.mObjects.last();

		for (auto& vertex : curMesh.Vertices) {
			// printf("{ %f, %f, %f }, \n", vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
			object->mTopology.Points.append(Vec3F{ vertex.Position.X, vertex.Position.Y, vertex.Position.Z });
			object->mTopology.Normals.append(Vec3F{ vertex.Normal.X, vertex.Normal.Y, vertex.Normal.Z });
		}

		for (int j = 0; j < curMesh.Indices.size(); j += 3) {
			uint idx1 = (int) curMesh.Indices[j];
			uint idx2 = (int) curMesh.Indices[j + 1];
			uint idx3 = (int) curMesh.Indices[j + 2];
			// printf("{ %i, %i, %i },\n", idx1, idx2, idx3);
			object->mTopology.Indexes.append({ idx1, idx2, idx3 });
		}

		if (object->mTopology.Normals.size() != object->mTopology.Points.size()) {
			printf("Logic error loading normals\n");
		}
	}

	return scene.mObjects.size();
}
