
#include "WavPlayer.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <math.h>

WavPlayer::WavPlayer() :
	handle(nullptr),
	playbackThread(nullptr),
	isPlaying(false),
	progress(0.0f) {
	if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		std::cerr << "Error: Couldn't open ALSA PCM device." << std::endl;
	}
}

WavPlayer::~WavPlayer() {
	stopSong();
	if (handle) {
		snd_pcm_close(handle);
	}
}

void WavPlayer::loadSong(const char* filePath) {
	stopSong();

	if (snd_pcm_prepare(handle) < 0) {
		std::cerr << "Error: Couldn't prepare PCM." << std::endl;
		return;
	}

	// TODO: Load and parse the WAV file
	// For simplicity, assume a 16-bit, mono WAV file with a 44.1 kHz sample rate
	// You need to handle loading and parsing of the WAV file here

	// Example:
	// std::ifstream file(filePath, std::ios::binary);
	// if (!file) {
	//     std::cerr << "Error: Could not open WAV file." << std::endl;
	//     return;
	// }
	// ... (load and parse WAV file)

	snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 1, 44100, 1, 500000);
}

void WavPlayer::playSong() {
	if (!isPlaying) {
		isPlaying = true;
		playbackThread = new std::thread(&WavPlayer::playbackThreadFunction, this);
	}
}

void WavPlayer::stopSong() {
	if (isPlaying) {
		isPlaying = false;
		if (playbackThread && playbackThread->joinable()) {
			playbackThread->join();
		}
		delete playbackThread;
		playbackThread = nullptr;
		snd_pcm_drop(handle);
	}
}

void WavPlayer::setVolume(float volume) {
	// TODO: Adjust volume using ALSA API
	// You might need to use functions like snd_pcm_volume() or other ALSA functions
}

float WavPlayer::getProgress() const { return progress; }

float WavPlayer::getDurationSec() const {
	// TODO: Return the duration of the loaded song
	// You need information from the loaded WAV file for this
	return 0.0f;
}

void WavPlayer::setProgress(float newProgress) {
	if (newProgress >= 0.0f && newProgress <= 1.0f) {
		progress = newProgress;

		// TODO: Adjust playback position based on the progress using ALSA API
		// You might need to use functions like snd_pcm_writei() or other ALSA functions
	}
}

void WavPlayer::playbackThreadFunction() {
	const int buffer_size = 1024; // Adjust the buffer size according to your requirements
	short buffer[buffer_size];

	while (isPlaying) {
		// TODO: Implement actual playback logic using ALSA API
		// You might need to use functions like snd_pcm_writei() or other ALSA functions

		// For simplicity, we'll fill the buffer with dummy data
		for (int i = 0; i < buffer_size; ++i) {
			buffer[i] = static_cast<short>(32767 * std::sin(2.0 * M_PI * 440.0 * progress));
		}

		// Write the buffer to the PCM device
		int err = snd_pcm_writei(handle, buffer, buffer_size);
		if (err < 0) {
			std::cerr << "Error writing to PCM device: " << snd_strerror(err) << std::endl;
			stopSong();
			return;
		}

		// Update progress
		progress += 0.1f;
		if (progress >= 1.0f) {
			stopSong();
		}

		// Sleep for a short duration (adjust as needed)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void example() {
	WavPlayer player;

	// Load a WAV file
	player.loadSong("path/to/your/file.wav");

	// Start playing the song asynchronously
	player.playSong();

	// Simulate a main application loop (replace this with your actual application logic)
	for (int i = 0; i < 30; ++i) {
		std::cout << "Progress: " << player.getProgress() << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	// Stop the song playback
	player.stopSong();
}