#pragma once

#include <iostream>
#include <thread>
#include <alsa/asoundlib.h>

class WavPlayer {
public:
    WavPlayer();
    ~WavPlayer();

    void loadSong(const char* filePath);
    void playSong();
    void stopSong();
    void setVolume(float volume);
    float getProgress() const;
    float getDurationSec() const;
    void setProgress(float newProgress);

private:
    void playbackThreadFunction();

    snd_pcm_t* handle;
    std::thread* playbackThread;
    bool isPlaying;
    float progress;
};

void example();