#pragma once
#include <SFML/Audio.hpp>
#include <optional>

enum Sfx { SFX_JUMPSCARE, SFX_SCREAM, SFX_BREATH, SFX_BELLS, SFX_STINGER, SFX_WHISPER, SFX_SLAM, SFX_HEARTBEAT, SFX_CRACK, SFX_COUNT };
enum Track { TRACK_NONE, TRACK_TITLE, TRACK_GAME, TRACK_DREAD, TRACK_CHASE, TRACK_DEATH, TRACK_ENDING, TRACK_COUNT };

class Audio {
	sf::Music tracks[TRACK_COUNT];           // indexed by Track (TRACK_NONE unused)
	float baseVolume[TRACK_COUNT] = {};
	sf::SoundBuffer buffers[SFX_COUNT];
	static const int CHANNELS = 8;
	std::optional<sf::Sound> channels[CHANNELS];
	int nextChannel = 0;
	Track current = TRACK_NONE;
public:
	Audio();
	void play(Sfx s, float volume = 100.f, float pitch = 1.f);
	void music(Track t);
	void pauseAll(bool paused);
	void setTension(float fear01);   // music drops back as fear (heartbeat) rises
};
