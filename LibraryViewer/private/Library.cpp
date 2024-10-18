
#include "Library.hpp"
#include "LocalConnection.hpp"

#include "picojson.h"
#include <filesystem>

using namespace tp;

#define AS_STR (int1*) trackProperty.second.to_str().c_str()
#define AS_INT std::stoll((int1*) trackProperty.second.to_str().c_str())
#define AS_BOOL trackProperty.second.evaluate_as_boolean()
#define PROP(name) trackProperty.first == #name

std::string getHome() {
	const char* envVarName = "LIB_VIEW_HOME";
	char* envVarValue = std::getenv(envVarName);
	if (envVarValue != nullptr) {
		std::string out;
		out = (int1*) (std::string(envVarValue) + "/").c_str();
		return out;
	} else {
		return "library/";
	}
}

std::string getSongLocalPath(SongId id) { return getHome() + "tracks/" + std::to_string((tp::alni) id); }

SONG_FORMAT getSongFormat(const std::string& localPath) {
	std::filesystem::path wavFormat((localPath + ".wav").c_str());
	std::filesystem::path flacFormat((localPath + ".flac").c_str());
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

bool Library::loadJson(const std::string& path) {
	LocalConnection libraryFile;
	Buffer<int1> libraryFileMem;

	if (!libraryFile.connect(LocalConnection::Location(path.c_str()), LocalConnection::Type(true))) return false;

	libraryFileMem.reserve(libraryFile.size());
	libraryFile.readBytes(libraryFileMem.getBuff(), libraryFile.size());

	std::string json = libraryFileMem.getBuff();
	picojson::value jsonNode;
	std::string err = picojson::parse(jsonNode, json);

	if (!err.empty()) {
		printf("Given path - %s\n", path.c_str());
		printf("Home for library is - '%s'. Set it in the env as LIB_VIEW_HOME \n", getHome().c_str());
		printf("Error parsing json library file - check your paths and files\n %s \n", err.c_str());
		return false;
	}

	auto& traks = jsonNode.get<picojson::array>();
	for (auto& trackNode : traks) {
		Track newTrack;

		auto& track = trackNode.get<picojson::object>();
		for (auto& trackProperty : track) {
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