
#include "Library.hpp"
#include "LocalConnection.hpp"

#include "picojson.h"
#include <filesystem>

namespace tp {
	static ModuleManifest* deps[] = { &gModuleGraphics, &gModuleConnection, nullptr };
	ModuleManifest gModuleLibraryViewer = ModuleManifest("LibraryViewer", nullptr, nullptr, deps);
}

using namespace tp;

#define AS_STR (int1*) trackProperty.second.to_str().c_str()
#define AS_INT std::stoll((int1*) trackProperty.second.to_str().c_str())
#define AS_BOOL trackProperty.second.evaluate_as_boolean()
#define PROP(name) trackProperty.first == #name

tp::String getHome() {
	const char* envVarName = "LIB_VIEW_HOME";
	char* envVarValue = std::getenv(envVarName);
	if (envVarValue != nullptr) {
		tp::String out;
		out = (int1*) (std::string(envVarValue) + "/").c_str();
		return  out;
	} else {
		return "";
	}
}

tp::String getSongLocalPath(SongId id) {
	return getHome() + "tracks/" + tp::String((tp::alni)id);
}

SONG_FORMAT getSongFormat(const String& localPath) {
	std::filesystem::path wavFormat((localPath + ".wav").read());
	std::filesystem::path flacFormat((localPath + ".flac").read());
	if (std::filesystem::exists(flacFormat)) return SONG_FORMAT::FLAC;
	if (std::filesystem::exists(wavFormat)) return SONG_FORMAT::WAV;
	return SONG_FORMAT::NONE;
}

void Library::checkExisting() {
	for (auto track : mTraks) {
		auto const path = getSongLocalPath(track->mId);
		track->mExists = getSongFormat(path) != SONG_FORMAT::NONE;
	}
}

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
			if (PROP(Name)) newTrack.mName = AS_STR;
			else if (PROP(Artist)) newTrack.mArtist = AS_STR;
			else if (PROP(Track ID)) newTrack.mId = AS_INT;
			else if (PROP(Album Artist)) newTrack.mAlbumArtist = AS_STR;
			else if (PROP(Composer)) newTrack.mComposer = AS_STR;
			else if (PROP(Album)) newTrack.mAlbum = AS_STR;
			else if (PROP(Genre)) newTrack.mGenre = AS_STR;
			else if (PROP(Size)) newTrack.mSize = AS_INT;
			else if (PROP(Total Time)) newTrack.mTotalTime = AS_INT;
			else if (PROP(Year)) newTrack.mYear = AS_INT;
			else if (PROP(Date Modified)) newTrack.mDateModified = AS_STR;
			else if (PROP(Date Added)) newTrack.mDateAdded = AS_STR;
			else if (PROP(Play Count)) newTrack.mPlayCount = AS_INT;
			else if (PROP(Play Date)) newTrack.mPlayDate = AS_STR;
			else if (PROP(Play Date UTC)) newTrack.mPlayDateUTC = AS_STR;
			else if (PROP(Skip Count)) newTrack.mSkipCount = AS_INT;
			else if (PROP(Skip Date)) newTrack.mSkipDate = AS_STR;
			else if (PROP(Release Date)) newTrack.mReleaseDate = AS_STR;
			else if (PROP(Album Rating)) newTrack.mAlbumRating = AS_INT;
			else if (PROP(Album Rating Computed)) newTrack.mAlbumRatingComputed = AS_BOOL;
			else if (PROP(Loved)) newTrack.mLoved = AS_BOOL;
			else if (PROP(Album Loved)) newTrack.mAlbumLoved = AS_BOOL;
			else if (PROP(Explicit)) newTrack.mExplicit = AS_BOOL;
		}

		mTraks.append(newTrack);
	}

	return true;
}