#include "Player.hpp"
#include "Buffer.hpp"
#include "Multithreading.hpp"

#include "portaudio.h"

class DeviceStream {
	friend class Player;

public:
	DeviceStream() {
		auto paStatus = Pa_Initialize() ;
		mInitializationStatus = paStatus == paNoError;
		if (!mInitializationStatus) {
			fprintf(stderr, "An error occurred while using the portaudio stream\n");
			fprintf(stderr, "Error number: %d\n", paStatus);
			fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(paStatus));
		}
	}

	~DeviceStream() {
		if (mInitializationStatus) {
			Pa_Terminate();
		}
	}

	bool openStream(const tp::halnf* buffer, const tp::ualni* pointerMax, tp::ualni sampleRate) {
		if (!mInitializationStatus) return false;

		mPointerMax = pointerMax;
		mBuffer = buffer;
		
		PaStreamParameters outputParameters;

		outputParameters.device = Pa_GetDefaultOutputDevice();
		if (outputParameters.device == paNoDevice) {
			return false;
		}

		const PaDeviceInfo* pInfo = Pa_GetDeviceInfo(outputParameters.device);
		if (pInfo != 0) {
			printf("Output device name: '%s'\r", pInfo->name);
		}

		outputParameters.channelCount = 2;
		outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
		outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
		outputParameters.hostApiSpecificStreamInfo = NULL;

		PaError err = Pa_OpenStream(&mPaStream, NULL, &outputParameters, sampleRate, 44100 * 0.2f, 0, &DeviceStream::paCallback, this);

		if (err != paNoError) {
			return false;
		}

		err = Pa_SetStreamFinishedCallback(mPaStream, &DeviceStream::paStreamFinished);

		if (err != paNoError) {
			Pa_CloseStream(mPaStream);
			mPaStream = 0;
			return false;
		}

		return true;
	}

	bool closeStream() {
		if (mPaStream == 0) return false;
		PaError err = Pa_CloseStream(mPaStream);

		mPaStream = 0;
		mBuffer = nullptr;
		mPointerMax = 0;
		mPointer = 0;

		return (err == paNoError);
	}

	bool continueStream() {
		if (mPaStream == 0) return false;
		PaError err = Pa_StartStream(mPaStream);
		return (err == paNoError);
	}

	bool freezeStream() {
		if (mPaStream == 0) return false;
		PaError err = Pa_StopStream(mPaStream);
		return (err == paNoError);
	}

	tp::ualni getCurrentFrame() const {
		return mPointer;
	}

	void setCurrentFrame(tp::ualni ptr) {
		ptr = tp::clamp(ptr, (tp::ualni) 0, *mPointerMax);
		mMutex.lock();
		mPointer = ptr;
		mMutex.unlock();
	}

private:
	int paCallbackMethod(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags) {
		if (mPointer + framesPerBuffer >= *mPointerMax) return paContinue;

		float* out = (float*) outputBuffer;
		mMutex.lock();
		for (unsigned long i = 0; i < framesPerBuffer; i++) {
			*out++ = (mBuffer)[mPointer * 2] * mVolume;
			*out++ = (mBuffer)[mPointer * 2 + 1] * mVolume;
			mPointer++;
		}
		mMutex.unlock();
		return paContinue;
	}

	void paStreamFinishedMethod() { printf("Stream Completed\n"); }

	static int paCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) { 
		return ((DeviceStream*) userData)->paCallbackMethod(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags); 
	}

	static void paStreamFinished(void* userData) { return ((DeviceStream*) userData)->paStreamFinishedMethod(); }

private:
	bool mInitializationStatus = false;
	PaStream* mPaStream = nullptr;
	tp::Mutex mMutex;

	const tp::halnf* mBuffer = nullptr;
	const tp::ualni* mPointerMax = nullptr;

	tp::ualni mPointer = 0;
	tp::halnf mVolume = 1.f;
};

Player::Player() {
	mDeviceStream = new DeviceStream();
}

Player::~Player() {
	delete mDeviceStream;
}

bool Player::startStreamTrack(SongId id) {
	mDeviceStream->closeStream();
	
	if (!mMusicStream.openStream(id))  {
		connected = 0;
		return false;
	}

	if (!mDeviceStream->openStream(mMusicStream.getBuffer(), mMusicStream.getProgressLive(), mMusicStream.getRate())) {
		connected = 0;
		return false;
	}

	mMusicStream.continueStream();
	
	connected = id;
	return true;
}

SongId Player::getPlayingId() const {
	return connected;
}

void Player::continuePlayback() {
	if (!connected) return;
	mDeviceStream->continueStream();
}

void Player::freezePlayback() {
	if (!connected) return;
	mDeviceStream->freezeStream();
}

tp::halnf Player::getPlaybackProgress() const { 
	if (!connected) return 0;
	return (tp::halnf) mDeviceStream->getCurrentFrame() / (tp::halnf) mMusicStream.getFramesLength(); 
}

tp::halnf Player::getDurationSec() const { return 0; }

void Player::setPlaybackProgress(tp::halnf val) {
	if (!connected) return;
	mDeviceStream->setCurrentFrame(val * mMusicStream.getFramesLength());
}

void Player::setVolume(tp::halnf vol) {
	if (!connected) return;
	mDeviceStream->mVolume = vol;
}

tp::halnf Player::getLoadProgress() const {
	if (!connected) return 0;
	return (tp::halnf) mMusicStream.getFramesLoaded() / (tp::halnf) mMusicStream.getFramesLength();
}

tp::halnf Player::getVolume() {
	if (!connected) return 0;
	return mDeviceStream->mVolume;
}