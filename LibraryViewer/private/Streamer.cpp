#include "Streamer.hpp"

#ifndef ENV_OS_WINDOWS
typedef u_int8_t uint8_t;
typedef u_int16_t uint16_t;
typedef u_int32_t uint32_t;
#endif

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

#include <thread>

class Context {
public:
	Context() = default;

	bool preloadInfo(const std::string& path, MusicStream* owner) {
		mOwner = owner;

		pFlac = drflac_open_file((path + ".flac").c_str(), NULL);

		if (pFlac == NULL) {
			return false;
		}

		if (pFlac->channels != 2) {
			drflac_close(pFlac);
			return false;
		}

		mOwner->mLengthFrames = pFlac->totalPCMFrameCount;
		mOwner->mChanels = pFlac->channels;
		mOwner->mRate = pFlac->sampleRate;

		mOwner->mBuffer.reserve(mOwner->mLengthFrames * 2);
		
		return true;
	}

	void runThread() {
		if (threadRunning) return;
		threadRunning = true;
		threadObj = std::thread(&Context::threadWorkStatic, this);
	}

	void stop() {
		if (threadRunning) {
			exitThread = true;
			threadObj.join();
			threadRunning = false;
			exitThread = false;
		}

		if (pFlac) drflac_close(pFlac);
		pFlac = nullptr;
	}

	~Context() {
		stop();
	}

private:
	std::thread threadObj;
	bool threadRunning = false;
	bool exitThread = false;

	void threadWork() {
		int framesRead = 0;
		while (!exitThread && (framesRead = drflac_read_pcm_frames_f32(pFlac, mOwner->mRate, mOwner->mBuffer.getBuff() + mOwner->mProgress * 2)) > 0) {
			mOwner->mProgress += framesRead;
			// drflac_seek_to_pcm_frame(pFlac, mOwner->mProgress);
			// tp::sleep(500);
		}
	}

	static void threadWorkStatic(void* p) { ((Context*) p)->threadWork(); }

public:
	drflac* pFlac = nullptr;
	MusicStream* mOwner;
};

MusicStream::MusicStream() {
	mContext = new Context();
}

MusicStream::~MusicStream() {
	delete mContext;
}

bool MusicStream::openStream(SongId id) {
	closeStream();
	auto const path = getSongLocalPath(id);
	auto format = getSongFormat(path);
	if (format != SONG_FORMAT::FLAC) return false;
	return mContext->preloadInfo(path, this);
}

void MusicStream::closeStream() {
	mContext->stop();
	mProgress = 0;
}

tp::halnf* MusicStream::getBuffer() {
	return mBuffer.getBuff();
}

tp::ualni* MusicStream::getProgressLive() {
	return &mProgress;
}

tp::uhalni MusicStream::getRate() const {
	return mRate;
}

void MusicStream::continueStream() {
	mContext->runThread();
}

tp::ualni MusicStream::getFramesLength() const {
	return mLengthFrames;
}

tp::ualni MusicStream::getFramesLoaded() const {
	return mProgress;
}