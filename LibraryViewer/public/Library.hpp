#pragma once

#include "Window.hpp"

typedef tp::ualni SongId;
enum class SONG_FORMAT { WAV, FLAC, NONE };

std::string getHome();
std::string getSongLocalPath(SongId id);
SONG_FORMAT getSongFormat(const std::string& path);

class Track {
public:
	Track() = default;

public:
	SongId mId = 0;
	std::string mName = "undef";
	std::string mArtist = "undef";
	std::string mAlbumArtist = "undef";
  std::string mComposer = "undef";
  std::string mAlbum = "undef";
  std::string mGenre = "undef";
  tp::ualni mSize = 0;
  tp::ualni mTotalTime = 0;
  tp::ualni mYear = 0;
  std::string mDateModified = "undef";
  std::string mDateAdded = "undef";
  tp::ualni mPlayCount = 0;
  std::string mPlayDate = "undef";
  std::string mPlayDateUTC = "undef";
  tp::ualni mSkipCount = 0;
  std::string mSkipDate = "undef";
  std::string mReleaseDate = "undef";
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
	bool loadJson(const std::string& path);
  void checkExisting();

public:
	tp::Buffer<Track> mTraks;
};