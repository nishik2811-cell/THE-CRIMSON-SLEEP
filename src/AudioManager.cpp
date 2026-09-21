#include "AudioManager.h"
#include <cmath>
#include <cstdlib>

static const unsigned int SAMPLE_RATE = 44100;
static const double PI_D = 3.14159265358979323846;

// Builds a sine tone with a simple attack/release envelope so it
// doesn't click at the start/end. If descend is true the pitch drops
// over the duration (used for the horror "stinger").
sf::SoundBuffer AudioManager::makeTone(double frequency, double durationSec, double amplitude, bool descend) {
    unsigned int sampleCount = (unsigned int)(durationSec * SAMPLE_RATE);
    std::int16_t* samples = new std::int16_t[sampleCount];

    for (unsigned int i = 0; i < sampleCount; i++) {
        double t = (double)i / SAMPLE_RATE;
        double freq = descend ? frequency * (1.0 - 0.5 * (t / durationSec)) : frequency;
        double value = std::sin(2.0 * PI_D * freq * t);

        // envelope: quick fade-in, longer fade-out
        double fadeIn = 0.02;
        double fadeOut = 0.35;
        double env = 1.0;
        if (t < fadeIn) env = t / fadeIn;
        else if (t > durationSec - fadeOut) env = (durationSec - t) / fadeOut;
        if (env < 0.0) env = 0.0;

        double sampleValue = value * amplitude * env * 32000.0;
        samples[i] = (std::int16_t)sampleValue;
    }

    sf::SoundBuffer buffer;
    (void)buffer.loadFromSamples(samples, sampleCount, 1, SAMPLE_RATE, { sf::SoundChannel::Mono });
    delete[] samples;
    return buffer;
}

// Filtered white noise burst - basis for whispers/door creaks. A
// simple one-pole low-pass filter tames it from harsh static into a
// duller, more unsettling hiss.
sf::SoundBuffer AudioManager::makeNoiseBurst(double durationSec, double amplitude, double lowPassAmount) {
    unsigned int sampleCount = (unsigned int)(durationSec * SAMPLE_RATE);
    std::int16_t* samples = new std::int16_t[sampleCount];

    double previous = 0.0;
    for (unsigned int i = 0; i < sampleCount; i++) {
        double t = (double)i / SAMPLE_RATE;
        double raw = ((double)(rand() % 2000) / 1000.0) - 1.0; // -1..1
        double filtered = previous * lowPassAmount + raw * (1.0 - lowPassAmount);
        previous = filtered;

        double fadeIn = 0.05;
        double fadeOut = 0.4;
        double env = 1.0;
        if (t < fadeIn) env = t / fadeIn;
        else if (t > durationSec - fadeOut) env = (durationSec - t) / fadeOut;
        if (env < 0.0) env = 0.0;

        double sampleValue = filtered * amplitude * env * 32000.0;
        samples[i] = (std::int16_t)sampleValue;
    }

    sf::SoundBuffer buffer;
    (void)buffer.loadFromSamples(samples, sampleCount, 1, SAMPLE_RATE, { sf::SoundChannel::Mono });
    delete[] samples;
    return buffer;
}

sf::SoundBuffer AudioManager::makeHeartbeat() {
    double durationSec = 0.9;
    unsigned int sampleCount = (unsigned int)(durationSec * SAMPLE_RATE);
    std::int16_t* samples = new std::int16_t[sampleCount];

    for (unsigned int i = 0; i < sampleCount; i++) {
        double t = (double)i / SAMPLE_RATE;
        // Two low thumps at t=0.0 and t=0.35, each a very short 55Hz burst.
        double value = 0.0;
        double thumpTimes[2] = { 0.0, 0.35 };
        for (int k = 0; k < 2; k++) {
            double dt = t - thumpTimes[k];
            if (dt >= 0.0 && dt < 0.12) {
                double env = 1.0 - (dt / 0.12);
                value += std::sin(2.0 * PI_D * 55.0 * dt) * env;
            }
        }
        samples[i] = (std::int16_t)(value * 0.9 * 32000.0);
    }

    sf::SoundBuffer buffer;
    (void)buffer.loadFromSamples(samples, sampleCount, 1, SAMPLE_RATE, { sf::SoundChannel::Mono });
    delete[] samples;
    return buffer;
}

sf::SoundBuffer AudioManager::makeClick() {
    return makeTone(900.0, 0.06, 0.5, false);
}

sf::SoundBuffer AudioManager::makeDoor() {
    return makeNoiseBurst(0.5, 0.6, 0.85);
}

AudioManager::AudioManager()
    : ambientDroneBuffer(makeTone(60.0, 3.0, 0.25, false)),
      stingerBuffer(makeNoiseBurst(0.6, 0.9, 0.2)),
      whisperBuffer(makeNoiseBurst(0.8, 0.35, 0.9)),
      heartbeatBuffer(makeHeartbeat()),
      clickBuffer(makeClick()),
      doorBuffer(makeDoor()),
      ambientSound(ambientDroneBuffer),
      sfxSound(clickBuffer),
      stingerSound(stingerBuffer)
{
    ambientSound.setLooping(true);
    ambientSound.setVolume(35.f);
}

void AudioManager::playAmbientDrone() {
    if (ambientSound.getStatus() != sf::Sound::Status::Playing) {
        ambientSound.play();
    }
}

void AudioManager::stopAmbient() {
    ambientSound.stop();
}

void AudioManager::playJumpscareStinger() {
    stingerSound.setBuffer(stingerBuffer);
    stingerSound.setVolume(100.f);
    stingerSound.play();
}

void AudioManager::playWhisper() {
    sfxSound.setBuffer(whisperBuffer);
    sfxSound.setVolume(60.f);
    sfxSound.play();
}

void AudioManager::playHeartbeat() {
    sfxSound.setBuffer(heartbeatBuffer);
    sfxSound.setVolume(80.f);
    sfxSound.play();
}

void AudioManager::playClick() {
    sfxSound.setBuffer(clickBuffer);
    sfxSound.setVolume(50.f);
    sfxSound.play();
}

void AudioManager::playDoor() {
    sfxSound.setBuffer(doorBuffer);
    sfxSound.setVolume(70.f);
    sfxSound.play();
}
