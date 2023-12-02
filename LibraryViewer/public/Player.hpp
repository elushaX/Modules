#pragma once

#include "Library.hpp"
#include "Streamer.hpp"

class DeviceStream;

class Player {
public:
	Player();
	~Player();

	bool startStreamTrack(SongId id);
	SongId getPlayingId() const;
	void continuePlayback();
	void freezePlayback();

	tp::halnf getDurationSec() const;

	void setVolume(tp::halnf volume);
	tp::halnf getVolume();

	tp::halnf getPlaybackProgress() const;
	void setPlaybackProgress(tp::halnf newProgress);

	tp::halnf getLoadProgress() const;

private:
	SongId connected = 0;
	MusicStream mMusicStream;
	DeviceStream* mDeviceStream;
};