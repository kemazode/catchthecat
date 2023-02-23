#include "sound.hpp"
#include <iostream>

#include <vector>
#include <fstream>
#include <cstring>
#include <thread>
#include <bit>

#include <AL/al.h>
#include <AL/alc.h>

static ALCdevice *openALDevice = nullptr;
static ALCcontext *openALcontext = nullptr;

#define CHECK_AL_ERRORS() check_al_errors(__FILE__, __LINE__)
#define CHECK_ALC_ERRORS() check_alc_errors(__FILE__, __LINE__, openALDevice)

static char* load_wav(const std::string& filename,
               std::uint8_t& channels,
               std::int32_t& sampleRate,
               std::uint8_t& bitsPerSample,
               ALsizei& size);
static bool load_wav_file_header(std::ifstream& file,
                          std::uint8_t& channels,
                          std::int32_t& sampleRate,
                          std::uint8_t& bitsPerSample,
                          ALsizei& size);
std::int32_t convert_to_int(char* buffer, std::size_t len);
static bool check_al_errors(const std::string& filename, const std::uint_fast32_t line);
static bool check_alc_errors(const std::string& filename, const std::uint_fast32_t line, ALCdevice* device);

void SoundSystem::init() {
    openALDevice = alcOpenDevice(nullptr);
    if (!CHECK_ALC_ERRORS()) return;

    openALcontext = alcCreateContext(openALDevice, nullptr);
    if (!CHECK_ALC_ERRORS()) return;

    ALCboolean contextMadeCurrent = false;
    contextMadeCurrent = alcMakeContextCurrent(openALcontext);
    if (!CHECK_ALC_ERRORS()) {
    std::cerr << "ERROR: Could not make audio context current" << std::endl;
    return;
    }
}

Sound::Sound(const char *file_name)
{
    std::uint8_t channels;
    std::int32_t sampleRate;
    std::uint8_t bitsPerSample;
    ALsizei soundSize;
    char* soundData;
    if (!(soundData = load_wav(file_name, channels, sampleRate, bitsPerSample, soundSize))) {
    std::cerr << "ERROR: Could not load wav" << std::endl;
    return;
    }

    alGenBuffers(1, &buffer);
    if (!CHECK_AL_ERRORS()) return;

    ALenum format;

    if(channels == 1 && bitsPerSample == 8)
    format = AL_FORMAT_MONO8;
    else if(channels == 1 && bitsPerSample == 16)
    format = AL_FORMAT_MONO16;
    else if(channels == 2 && bitsPerSample == 8)
    format = AL_FORMAT_STEREO8;
    else if(channels == 2 && bitsPerSample == 16)
    format = AL_FORMAT_STEREO16;
    else
    {
    std::cerr
        << "ERROR: unrecognised wave format: "
        << channels << " channels, "
        << bitsPerSample << " bps" << std::endl;
    return;
    }

    alBufferData(buffer, format, soundData, soundSize, sampleRate);
    if (!CHECK_AL_ERRORS()) return;

    free(soundData);    
}

Source::Source(Sound& sound, ALfloat x, ALfloat y, ALfloat z)
{    
    alGenSources(1, &buffer);
    if (!CHECK_AL_ERRORS()) return;

    alSourcef(buffer, AL_PITCH, 1);
    if (!CHECK_AL_ERRORS()) return;

    alSourcef(buffer, AL_GAIN, 1.0f);
    if (!CHECK_AL_ERRORS()) return;

    alSource3f(buffer, AL_POSITION, x, y, z);
    if (!CHECK_AL_ERRORS()) return;

    alSource3f(buffer, AL_VELOCITY, 0, 0, 0);
    if (!CHECK_AL_ERRORS()) return;

    alSourcei(buffer, AL_LOOPING, AL_FALSE);
    if (!CHECK_AL_ERRORS()) return;

    alSourcei(buffer, AL_BUFFER, sound.buffer);
    if (!CHECK_AL_ERRORS()) return;

}

void Source::move(ALfloat x, ALfloat y, ALfloat z) {
    alSource3f(buffer, AL_POSITION, x, y, z);
    if (!CHECK_AL_ERRORS()) return;
}

//void Sound::playSource(Source_Info source) {
////    alSourcei(source.buffer, AL_BUFFER, sound.buffer);
////    if (!CHECK_AL_ERRORS()) return;

//}

SoundSystem::~SoundSystem() {
    alcMakeContextCurrent(nullptr);
    CHECK_ALC_ERRORS();

    alcDestroyContext(openALcontext);
    CHECK_ALC_ERRORS();

    ALCboolean closed;
    closed = alcCloseDevice(openALDevice);
    if (!closed)
    std::cerr << "ERROR: Could not close an audio device\n";
    CHECK_ALC_ERRORS();
}

std::int32_t convert_to_int(char* buffer, std::size_t len)
{
    std::int32_t a = 0;
    if(std::endian::native == std::endian::little)
    std::memcpy(&a, buffer, len);
    else
    for(std::size_t i = 0; i < len; ++i)
        reinterpret_cast<char*>(&a)[3 - i] = buffer[i];
    return a;
}

bool load_wav_file_header(std::ifstream& file,
                          std::uint8_t& channels,
                          std::int32_t& sampleRate,
                          std::uint8_t& bitsPerSample,
                          ALsizei& size)
{
    char buffer[4];
    if(!file.is_open())
        return false;

    // the RIFF
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read RIFF" << std::endl;
        return false;
    }
    if(std::strncmp(buffer, "RIFF", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (header doesn't begin with RIFF)" << std::endl;
        return false;
    }

    // the size of the file
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read size of file" << std::endl;
        return false;
    }

    // the WAVE
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read WAVE" << std::endl;
        return false;
    }
    if(std::strncmp(buffer, "WAVE", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (header doesn't contain WAVE)" << std::endl;
        return false;
    }

    // "fmt/0"
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read fmt/0" << std::endl;
        return false;
    }

    // this is always 16, the size of the fmt data chunk
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read the 16" << std::endl;
        return false;
    }

    // PCM should be 1?
    if(!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read PCM" << std::endl;
        return false;
    }

    // the number of channels
    if(!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read number of channels" << std::endl;
        return false;
    }
    channels = convert_to_int(buffer, 2);

    // sample rate
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read sample rate" << std::endl;
        return false;
    }
    sampleRate = convert_to_int(buffer, 4);

    // (sampleRate * bitsPerSample * channels) / 8
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read (sampleRate * bitsPerSample * channels) / 8" << std::endl;
        return false;
    }

    // ?? dafaq
    if(!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read dafaq" << std::endl;
        return false;
    }

    // bitsPerSample
    if(!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read bits per sample" << std::endl;
        return false;
    }
    bitsPerSample = convert_to_int(buffer, 2);

    // data chunk header "data"
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read data chunk header" << std::endl;
        return false;
    }
    if(std::strncmp(buffer, "data", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (doesn't have 'data' tag)" << std::endl;
        return false;
    }

    // size of data
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read data size" << std::endl;
        return false;
    }
    size = convert_to_int(buffer, 4);

    /* cannot be at the end of file */
    if(file.eof())
    {
        std::cerr << "ERROR: reached EOF on the file" << std::endl;
        return false;
    }
    if(file.fail())
    {
        std::cerr << "ERROR: fail state set on the file" << std::endl;
        return false;
    }

    return true;
}

char* load_wav(const std::string& filename,
               std::uint8_t& channels,
               std::int32_t& sampleRate,
               std::uint8_t& bitsPerSample,
               ALsizei& size)
{
    std::ifstream in(filename, std::ios::binary);
    if(!in.is_open())
    {
        std::cerr << "ERROR: Could not open \"" << filename << "\"" << std::endl;
    return nullptr;
    }
    if(!load_wav_file_header(in, channels, sampleRate, bitsPerSample, size))
    {
    std::cerr << "ERROR: Could not load wav header of \"" << filename << "\"" << std::endl;
    return nullptr;
    }

    char* data = new char[size];

    in.read(data, size);

    return data;
}

bool check_al_errors(const std::string& filename, const std::uint_fast32_t line)
{
    ALenum error = alGetError();
    if(error != AL_NO_ERROR)
    {
    std::cerr << "***ERROR*** (" << filename << ": " << line << ")\n" ;
    switch(error)
    {
    case AL_INVALID_NAME:
        std::cerr << "AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL function";
        break;
    case AL_INVALID_ENUM:
        std::cerr << "AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL function";
        break;
    case AL_INVALID_VALUE:
        std::cerr << "AL_INVALID_VALUE: an invalid value was passed to an OpenAL function";
        break;
    case AL_INVALID_OPERATION:
        std::cerr << "AL_INVALID_OPERATION: the requested operation is not valid";
        break;
    case AL_OUT_OF_MEMORY:
        std::cerr << "AL_OUT_OF_MEMORY: the requested operation resulted in OpenAL running out of memory";
        break;
    default:
        std::cerr << "UNKNOWN AL ERROR: " << error;
    }
    std::cerr << std::endl;
    return false;
    }
    return true;
}

bool check_alc_errors(const std::string& filename, const std::uint_fast32_t line, ALCdevice* device)
{
    ALCenum error = alcGetError(device);
    if(error != ALC_NO_ERROR)
    {
        std::cerr << "***ERROR*** (" << filename << ": " << line << ")\n" ;
        switch(error)
        {
        case ALC_INVALID_VALUE:
            std::cerr << "ALC_INVALID_VALUE: an invalid value was passed to an OpenAL function";
            break;
        case ALC_INVALID_DEVICE:
            std::cerr << "ALC_INVALID_DEVICE: a bad device was passed to an OpenAL function";
            break;
        case ALC_INVALID_CONTEXT:
            std::cerr << "ALC_INVALID_CONTEXT: a bad context was passed to an OpenAL function";
            break;
        case ALC_INVALID_ENUM:
            std::cerr << "ALC_INVALID_ENUM: an unknown enum value was passed to an OpenAL function";
            break;
        case ALC_OUT_OF_MEMORY:
            std::cerr << "ALC_OUT_OF_MEMORY: an unknown enum value was passed to an OpenAL function";
            break;
        default:
            std::cerr << "UNKNOWN ALC ERROR: " << error;
        }
        std::cerr << std::endl;
        return false;
    }
    return true;
}

void Source::play() {
    alSourcePlay(buffer);
    if (!CHECK_AL_ERRORS()) return;

    ALint state = AL_PLAYING;

    while (state == AL_PLAYING) {
        alGetSourcei(buffer, AL_SOURCE_STATE, &state);
        if (!CHECK_AL_ERRORS()) return;
    }
}
void Source::playAsync() {
    std::thread(&Source::play, this).detach();
}
