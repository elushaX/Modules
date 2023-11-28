#pragma once

#include "Window.hpp"

namespace tp {
	extern ModuleManifest gModuleLibraryViewer;
}

class Track {
public:
	Track() = default;

public:
	tp::String mName = "undef";
	tp::String mArtist = "undef";
};

class Library {
public:
	Library() = default;

public:
	bool loadJson(const tp::String& path);

public:
	tp::Buffer<Track> mTraks;
};

class LibraryViewverGui {};

void runApp();