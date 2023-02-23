#ifndef SOUND_HPP
#define SOUND_HPP

#include <AL/al.h>

class Sound {
    ALuint buffer;
    friend class Source;

public:
    Sound(const char *file_wav);
    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;

    Sound(Sound &&) = default;

    ~Sound() {
        alDeleteBuffers(1, &buffer);
    }
};

class Source {
    ALuint buffer;

public:
    Source(Sound& sound, ALfloat x, ALfloat y, ALfloat z);
    Source(const Source&) = delete;
    Source& operator=(const Source&) = delete;

    Source(Source &&) = default;
    void play();
    void playAsync();
    void move(ALfloat x, ALfloat y, ALfloat z);

    ~Source() {
        alDeleteSources(1, &buffer);
    }
};

class SoundSystem {
private:
    SoundSystem() = default;

    SoundSystem(const SoundSystem&) = delete;
    SoundSystem& operator=(const SoundSystem&) = delete;
public:

    void init();

    static SoundSystem& getInstance() {
        static SoundSystem instance;
        return instance;
    };

    ~SoundSystem();
};

#endif // SOUND_HPP
