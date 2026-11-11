#pragma once
#include <SFML/Audio.hpp>
#include "ds/Str.h"

// Every sound effect may have several takes; one is picked at random.
enum Sfx {
	SFX_JUMPSCARE, SFX_SCREAM, SFX_STING, SFX_WHISPER, SFX_SLAM, SFX_BREATH, SFX_CRACK, SFX_MOAN, SFX_GLASS,
	SFX_HEART_SLOW, SFX_HEART_FAST, SFX_HURT, SFX_GASP, SFX_DEATH,
	SFX_DOOR_OPEN, SFX_DOOR_LOCKED, SFX_UNLOCK, SFX_KEYS, SFX_PICKUP, SFX_PAPER, SFX_LEVER, SFX_CHIME, SFX_FLARE,
	SFX_BRIDE_APPEAR, SFX_BRIDE, SFX_CHILD, SFX_CHILD_LUNGE,
	SFX_STEP_CARPET, SFX_STEP_WOOD, SFX_STEP_CREAK, SFX_STEP_STONE, SFX_STEP_WATER,
	SFX_UI_MOVE, SFX_UI_CLICK,
	SFX_COUNT
};
enum Track { TRACK_NONE, TRACK_TITLE, TRACK_GAME, TRACK_CHASE, TRACK_DEATH, TRACK_END_TRUE, TRACK_END_OTHER, TRACK_END_TRAPPED, TRACK_COUNT };

class Audio {
	static const int MAX_TAKES = 6;
	sf::SoundBuffer buffers[SFX_COUNT][MAX_TAKES];
	int takes[SFX_COUNT] = {};
	static const int CHANNELS = 12;
	sf::Sound* channels[CHANNELS] = {};    // reused round-robin (sf::Sound needs a buffer to exist)
	int nextChannel = 0;

	sf::Music tracks[TRACK_COUNT];          // title / chase / death / endings
	float trackVolume[TRACK_COUNT] = {};
	sf::Music themes[3];                    // in-game music: calm rooms / dangerous rooms / the Nursery
	int themeIdx = 0;
	sf::Music& theme() { return themes[themeIdx]; }
	sf::Music pauseMusic;                   // plays while the pause menu is open
	bool inPause = false;
	bool muted = false;
	sf::Music roomAmb, roomLayer;           // the current room's ambience + optional extra loop
	Str ambName, layerName;
	Track current = TRACK_NONE;
	float tension = 0;
	void startRoom();
	void stopRoom();
public:
	Audio();
	~Audio() { for (int i = 0; i < CHANNELS; ++i) delete channels[i]; }
	Audio(const Audio&) = delete;
	Audio& operator=(const Audio&) = delete;
	void play(Sfx s, float volume = 100.f, float pitch = 1.f);
	void music(Track t);
	enum Theme { THEME_CALM, THEME_DREAD, THEME_NURSERY };
	void setRoom(const char* ambience, const char* layer, int theme);   // files in assets/audio, layer may be ""
	void setPaused(bool paused);     // swap the game music for the pause track (and back)
	void toggleMute();
	bool isMuted() const { return muted; }
	void pauseAll(bool paused);
	void setTension(float fear01);   // room ambience drops back as fear (heartbeat) rises
};

// So menus can click without owning the audio object.
extern Audio* g_audio;
