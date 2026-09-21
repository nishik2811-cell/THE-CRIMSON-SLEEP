#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <SFML/Audio.hpp>
#include <string>

// ---------------------------------------------------------------------
// AudioManager
//
// Uses real royalty-free horror sound effects bundled in
// assets/Sounds/ (see README for credits/sources) for the scary
// moments - ambient drone, jumpscare screams, whispers, trap stings,
// menu music, and ending music. Every real file has a procedurally
// generated waveform as a silent-failure fallback, so the game still
// runs (just less scary) if a sound file is ever missing - loading
// never throws or crashes the game.
//
// This class is pure SFML presentation code - it has no bearing on
// the data-structure requirements of the project.
// ---------------------------------------------------------------------
class AudioManager {
private:
    sf::SoundBuffer ambientDroneBuffer;   // universfield-scary-music-box (loops during gameplay)
    sf::SoundBuffer stingerBuffer;        // judgecruz1 loud screaming (Ghost Attack / Game Over)
    sf::SoundBuffer whisperBuffer;        // freesound_community scary-breath (Dark Room / Ghost Nearby / whispers)
    sf::SoundBuffer trapBuffer;           // hgoliya08 scary-sound-effect (Trap threat)
    sf::SoundBuffer eventBuffer;          // dragon-studio scary-bells (haunted events: door slam, object falls...)
    sf::SoundBuffer screamBuffer;         // freesound_community scream (secondary ghost scare)
    sf::SoundBuffer menuMusicBuffer;      // freesound_community scary-piano-music (main menu loop)
    sf::SoundBuffer endingMusicBuffer;    // matthewvakaliuk73627 music-box-scary (ending screens)
    sf::SoundBuffer heartbeatBuffer;
    sf::SoundBuffer clickBuffer;
    sf::SoundBuffer doorBuffer;

    sf::Sound ambientSound;
    sf::Sound menuMusicSound;
    sf::Sound sfxSound;
    sf::Sound stingerSound;
    sf::Sound screamSound;
    sf::Sound endingSound;

    static sf::SoundBuffer makeTone(double frequency, double durationSec, double amplitude, bool descend);
    static sf::SoundBuffer makeNoiseBurst(double durationSec, double amplitude, double lowPassAmount);
    static sf::SoundBuffer makeHeartbeat();
    static sf::SoundBuffer makeClick();
    static sf::SoundBuffer makeDoor();

    // Tries assets/Sounds/<filename> then ../assets/Sounds/<filename>;
    // returns fallback unchanged if the file can't be found/decoded.
    static sf::SoundBuffer loadOrFallback(const std::string& filename, const sf::SoundBuffer& fallback);

public:
    AudioManager();

    void playAmbientDrone();   // looping horror music - starts on entering PLAYING
    void stopAmbient();
    void playMenuMusic();      // looping creepy piano - starts on the Main Menu
    void stopMenuMusic();
    void playJumpscareStinger();
    void playScream();         // secondary scare (Ghost Nearby)
    void playWhisper();
    void playTrapHit();
    void playEventStinger();
    void playEndingMusic();
    void playHeartbeat();
    void playClick();
    void playDoor();
};

#endif
