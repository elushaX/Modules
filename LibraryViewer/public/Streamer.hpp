#pragma once

#include "Library.hpp"

class Context;

class MusicStream {
	friend Context;

public:
	MusicStream();
	~MusicStream();

	bool openStream(SongId id);
  void closeStream();
  
	tp::halnf* getBuffer();
	tp::ualni* getProgressLive();
	tp::uhalni getRate() const;

	void continueStream();
	tp::ualni getFramesLength() const;
	tp::ualni getFramesLoaded() const;

public:
	class Context* mContext = nullptr;
	// tp::Mutex mMutex;

	tp::ualni mLengthFrames = 0;
	tp::ualni mChanels = 0;
	tp::ualni mRate = 0;
	tp::ualni mProgress;
	tp::Buffer<tp::halnf> mBuffer;
};