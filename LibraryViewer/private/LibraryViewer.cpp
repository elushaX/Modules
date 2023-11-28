
#include "LibraryViewer.hpp"
#include "LocalConnection.hpp"

#include "picojson.h"

namespace tp {
	static ModuleManifest* deps[] = { &gModuleGraphics, &gModuleConnection, nullptr };
	ModuleManifest gModuleLibraryViewer = ModuleManifest("LibraryViewer", nullptr, nullptr, deps);
}

using namespace tp;

bool Library::loadJson(const String& path) {
	LocalConnection libraryFile;
	Buffer<int1> libraryFileMem;

	if (!libraryFile.connect(LocalConnection::Location(path), LocalConnection::Type(true))) return false;

	libraryFileMem.reserve(libraryFile.size());
	libraryFile.readBytes(libraryFileMem.getBuff(), libraryFile.size());
	
	std::string json = libraryFileMem.getBuff();
	picojson::value jsonNode;
	std::string err = picojson::parse(jsonNode, json);
	
	if (! err.empty()) {
		return false;
	}

	auto & traks = jsonNode.get<picojson::array>();
	for (auto & trackNode : traks) {
		Track newTrack;

		auto & track = trackNode.get<picojson::object>();
		for (auto & trackProperty : track) {
			if (trackProperty.first == "Name") {
				newTrack.mName = (int1*) trackProperty.second.to_str().c_str();
			}
			else if (trackProperty.first == "Artist") {
				newTrack.mArtist = (int1*) trackProperty.second.to_str().c_str();
			}
		}

		mTraks.append(newTrack);
	}

	return true;
}