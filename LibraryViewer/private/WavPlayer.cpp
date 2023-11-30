#include "WavPlayer.hpp"

#include <math.h>
#include <stdio.h>

#include "portaudio.h"
#include "Buffer.hpp"
#include "Multithreading.hpp"

typedef u_int8_t uint8_t;
typedef u_int16_t uint16_t;
typedef u_int32_t uint32_t;

#include "wav_file_reader.h"

class Sine {
	friend class MusicPlayerContext;
public:
	Sine() {}

	void setSampleRate(int aSampleRate) {
		sampleRate = aSampleRate;
	}

	tp::Buffer<float>* geLeftBuffer() { return &leftBuffer; }
	tp::Buffer<float>* geRightBuffer() { return &rightBuffer; }

	bool open(PaDeviceIndex index) {
		PaStreamParameters outputParameters;

		outputParameters.device = index;
		if (outputParameters.device == paNoDevice) {
			return false;
		}

		const PaDeviceInfo* pInfo = Pa_GetDeviceInfo(index);
		if (pInfo != 0) {
			printf("Output device name: '%s'\r", pInfo->name);
		}

		outputParameters.channelCount = 2;
		outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
		outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
		outputParameters.hostApiSpecificStreamInfo = NULL;

		PaError err = Pa_OpenStream(&stream, NULL, &outputParameters, sampleRate, paFramesPerBufferUnspecified, paClipOff, &Sine::paCallback, this);

		/* Failed to open stream to device !!! */
		if (err != paNoError) {
			return false;
		}

		err = Pa_SetStreamFinishedCallback(stream, &Sine::paStreamFinished);

		if (err != paNoError) {
			Pa_CloseStream(stream);
			stream = 0;
			return false;
		}

		return true;
	}

	bool close() {
		if (stream == 0) return false;
		PaError err = Pa_CloseStream(stream);
		stream = 0;
		return (err == paNoError);
	}

	bool start() {
		if (stream == 0) return false;
		PaError err = Pa_StartStream(stream);
		return (err == paNoError);
	}

	bool stop() {
		if (stream == 0) return false;
		PaError err = Pa_StopStream(stream);
		return (err == paNoError);
	}

private:
	int paCallbackMethod(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags) {
		float* out = (float*) outputBuffer;

		mMutex.lock();
		for (unsigned long i = 0; i < framesPerBuffer; i++) {
			*out++ = leftBuffer[leftSampleIdx];
			*out++ = rightBuffer[rightSampleIdx];
			
			leftSampleIdx += 1;
			rightSampleIdx += 1;

			if (leftSampleIdx >= leftBuffer.size()) 
				leftSampleIdx -= 1;
			if (rightSampleIdx >= rightBuffer.size()) 
				rightSampleIdx -= 1;
		}
		mMutex.unlock();

		return paContinue;
	}

	void paStreamFinishedMethod() { printf("Stream Completed\n"); }

	static int paCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) { 
		return ((Sine*) userData)->paCallbackMethod(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags); 
	}

	static void paStreamFinished(void* userData) { return ((Sine*) userData)->paStreamFinishedMethod(); }

	PaStream* stream = 0;

	tp::Buffer<float> leftBuffer;
	tp::Buffer<float> rightBuffer;

	int leftSampleIdx = 0;
	int rightSampleIdx = 0;

	int sampleRate = 44100;

	tp::Mutex mMutex;
};

class MusicPlayerContext {
public:
	MusicPlayerContext() {
		initStatus = Pa_Initialize();
		if (initStatus != paNoError) {
			fprintf(stderr, "An error occurred while using the portaudio stream\n");
			fprintf(stderr, "Error number: %d\n", initStatus);
			fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(initStatus));
		}
	}

	bool load(SongId id) {
		if (connected) {
			stop();
			sine.close();
		}

		if (loadWav(id)) {
			connected = sine.open(Pa_GetDefaultOutputDevice());
			return true;
		}

		return false;
	}

	void stop() {
		if (!connected) return;
		sine.stop();
	}

	void start() {
		if (!connected) return;
		sine.start();
	}

	~MusicPlayerContext() {
		if (connected) {
			stop();
			sine.close();
		}

		if (initStatus == paNoError) {
			Pa_Terminate();
		}
	}

	bool loadWav(SongId id) {
		AudioFile<double> audioFile;
		
		if (!audioFile.load(std::string(getHome().read()) + std::string("traks/") + std::to_string(id) + ".wav")) {
			return false;
		}

		audioFile.printSummary();

		sine.setSampleRate(audioFile.getSampleRate());

		int numSamples = audioFile.getNumSamplesPerChannel();

		auto left = sine.geLeftBuffer();
		auto right = sine.geRightBuffer();

		left->reserve(numSamples);
		right->reserve(numSamples);

		int channel = 0;
		for (int i = 0; i < numSamples; i++) {
			left->getBuff()[i] = audioFile.samples[0][i];
			right->getBuff()[i] = audioFile.samples[1][i];
		}

		return true;
	}

	float getProgress() const { 
		return float(sine.leftSampleIdx) / sine.leftBuffer.size(); 
	}

	void setProgress(float newProgress) {
		newProgress = tp::clamp(newProgress, 0.f, 1.f);
		sine.mMutex.lock();
		sine.leftSampleIdx = sine.leftBuffer.size() * newProgress;
		sine.rightSampleIdx =  sine.leftBuffer.size() * newProgress;
		sine.mMutex.unlock();
	}

private:
	Sine sine;
	PaError initStatus;
	bool connected = false;
};

TrackPlayer::TrackPlayer() {
	mContext = new MusicPlayerContext();
}

TrackPlayer::~TrackPlayer() {
	delete mContext;
}

bool TrackPlayer::startStreamTrack(SongId id) {
	return mContext->load(id);
}

void TrackPlayer::playSong() {
	mContext->start();
}

void TrackPlayer::stopSong() {
	mContext->stop();
}

void TrackPlayer::setVolume(float volume) {}

float TrackPlayer::getProgress() const { 
	return mContext->getProgress(); 
}

float TrackPlayer::getDurationSec() const { return 0; }

void TrackPlayer::setProgress(float newProgress) {
	mContext->setProgress(newProgress);
}