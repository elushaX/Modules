#include "WavPlayer.hpp"

#include <math.h>
#include <stdio.h>

#include "portaudio.h"
#include "Buffer.hpp"

typedef u_int8_t uint8_t;
typedef u_int16_t uint16_t;
typedef u_int32_t uint32_t;

#include "wav_file_reader.h"

class Sine {
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

	void load() {
		if (connected) {
			stop();
			sine.close();
		}

		exampleLoad();
		connected = sine.open(Pa_GetDefaultOutputDevice());
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

	void exampleLoad() {
		AudioFile<double> audioFile;
		audioFile.load("library/Example.wav");

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

void TrackPlayer::startStreamTrack(SongId id) {
	mContext->load();
}

void TrackPlayer::playSong() {
	mContext->start();
}

void TrackPlayer::stopSong() {
	mContext->stop();
}

void TrackPlayer::setVolume(float volume) {}

float TrackPlayer::getProgress() const { return 0; }

float TrackPlayer::getDurationSec() const { return 0; }

void TrackPlayer::setProgress(float newProgress) {}