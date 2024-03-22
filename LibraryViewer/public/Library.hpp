#pragma once

#include "Window.hpp"
#include "Strings.hpp"

typedef tp::ualni SongId;
enum class SONG_FORMAT { WAV, FLAC, NONE };

tp::String getHome();
tp::String getSongLocalPath(SongId id);
SONG_FORMAT getSongFormat(const tp::String& path);

class Track {
public:
	Track() = default;

public:
	SongId mId = 0;
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
  bool mExists = false;
};

class Library {
public:
	Library() = default;

public:
	bool loadJson(const tp::String& path);
  void checkExisting();

public:
	tp::Buffer<Track> mTraks;
};