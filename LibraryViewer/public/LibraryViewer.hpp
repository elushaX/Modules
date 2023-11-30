#pragma once

#include "Window.hpp"

tp::String getHome();

namespace tp {
	extern ModuleManifest gModuleLibraryViewer;
}

class Track {
public:
	Track() = default;

public:
	tp::ualni mId = 0;
	tp::String mName = "undef";
	tp::String mArtist = "undef";
	tp::String mAlbumArtist = "undef";
  tp::String mComposer = "undef";
  tp::String mAlbum = "undef";
  tp::String mGenre = "undef";
  tp::ualni mSize = 0;
  tp::ualni mTotalTime = 0;
  tp::ualni mYear = 0;
  tp::String mDateModified = "undef";
  tp::String mDateAdded = "undef";
  tp::ualni mPlayCount = 0;
  tp::String mPlayDate = "undef";
  tp::String mPlayDateUTC = "undef";
  tp::ualni mSkipCount = 0;
  tp::String mSkipDate = "undef";
  tp::String mReleaseDate = "undef";
  tp::ualni mAlbumRating = 0;
  bool mAlbumRatingComputed = false;
  bool mLoved = false;
  bool mAlbumLoved = false;
  bool mExplicit = false;
  // tp::String mKind = "undef";
  // tp::ualni mDiscNumber = 0;
  // tp::ualni mDiscCount = 0;
  // tp::ualni mTrackNumber = 0;
  // tp::ualni mTrackCount = 0;
  // tp::ualni mBitRate = 0;
  // tp::ualni mSampleRate = 0;
  // tp::ualni Compilation
  // tp::ualni Artwork Count
  // tp::ualni Sort Album
  // tp::ualni Sort Artist
  // tp::ualni Sort Name
  // tp::ualni Persistent ID
  // tp::ualni Track Type
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