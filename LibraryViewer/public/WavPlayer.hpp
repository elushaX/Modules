#pragma once

#include "Common.hpp"

class MusicPlayerContext;
typedef tp::ualni SongId;

class TrackPlayer {
public:
	TrackPlayer();
	~TrackPlayer();

	void startStreamTrack(SongId id);

	void playSong();

	void stopSong();

	void setVolume(float volume);

	float getProgress() const;

	float getDurationSec() const;

	void setProgress(float newProgress);

private:
	MusicPlayerContext* mContext;
};