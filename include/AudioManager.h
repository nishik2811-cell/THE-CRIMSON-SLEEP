#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <SFML/Audio.hpp>

// ---------------------------------------------------------------------
// AudioManager
//
// The project must not depend on downloaded assets, so every sound
// effect here is generated in code (simple sine/noise waveforms with
// an amplitude envelope) instead of being loaded from a file. This
// class is pure SFML presentation code - it has no bearing on the
// data-structure requirements of the project.
// ---------------------------------------------------------------------
class AudioManager {
private:
    sf::SoundBuffer ambientDroneBuffer;
    sf::SoundBuffer stingerBuffer;
    sf::SoundBuffer whisperBuffer;
    sf::SoundBuffer heartbeatBuffer;
    sf::SoundBuffer clickBuffer;
    sf::SoundBuffer doorBuffer;

    sf::Sound ambientSound;
    sf::Sound sfxSound;
    sf::Sound stingerSound;

    static sf::SoundBuffer makeTone(double frequency, double durationSec, double amplitude, bool descend);
    static sf::SoundBuffer makeNoiseBurst(double durationSec, double amplitude, double lowPassAmount);
    static sf::SoundBuffer makeHeartbeat();
    static sf::SoundBuffer makeClick();
    static sf::SoundBuffer makeDoor();

public:
    AudioManager();

    void playAmbientDrone();  // low, looping horror hum - starts on entering PLAYING
    void stopAmbient();
    void playJumpscareStinger();
    void playWhisper();
    void playHeartbeat();
    void playClick();
    void playDoor();
};

#endif
