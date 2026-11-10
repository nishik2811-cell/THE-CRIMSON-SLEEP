#include "audio.h"
#include "globals.h"

Audio::Audio() {
	struct { Track t; const char* file; float volume; } list[] = {
		{ TRACK_TITLE,  "music_title.ogg",   80 },   // title / menus
		{ TRACK_GAME,   "music_explore.ogg", 70 },   // calmer rooms
		{ TRACK_DREAD,  "music_dread.ogg",   75 },   // the most dangerous rooms
		{ TRACK_CHASE,  "music_chase.ogg",   85 },   // something is hunting you
		{ TRACK_DEATH,  "music_death.ogg",   80 },   // you died / trapped ending
		{ TRACK_ENDING, "music_ending.ogg",  85 },   // endings
	};
	for (auto& e : list) {
		if (tracks[e.t].openFromFile(asset(std::string("audio/") + e.file))) {
			tracks[e.t].setLooping(true);
			tracks[e.t].setVolume(e.volume);
		}
		baseVolume[e.t] = e.volume;
	}
	const char* files[SFX_COUNT] = { "jumpscare.ogg", "scream.ogg", "breath.ogg", "bells.ogg", "stinger.ogg", "whisper.ogg", "slam.ogg", "heartbeat.ogg", "crack.wav" };
	for (int i = 0; i < SFX_COUNT; ++i) (void)buffers[i].loadFromFile(asset(std::string("audio/") + files[i]));
}

void Audio::play(Sfx s, float volume, float pitch) {
	std::optional<sf::Sound>& ch = channels[nextChannel];
	nextChannel = (nextChannel + 1) % CHANNELS;
	if (ch) ch->stop();
	ch.emplace(buffers[s]);
	ch->setVolume(volume);
	ch->setPitch(pitch);
	ch->play();
}

void Audio::music(Track t) {
	if (t == current) return;
	if (current != TRACK_NONE) {
		// Exploration tracks pause (so they resume where they were after a
		// chase); everything else restarts next time.
		if (current == TRACK_GAME || current == TRACK_DREAD) tracks[current].pause();
		else tracks[current].stop();
	}
	if (t == TRACK_TITLE || t == TRACK_ENDING || t == TRACK_DEATH) {       // leaving play: reset exploration
		tracks[TRACK_GAME].stop(); tracks[TRACK_DREAD].stop();
	}
	current = t;
	if (current != TRACK_NONE) {
		tracks[current].setVolume(baseVolume[current]);
		tracks[current].play();
	}
}

void Audio::pauseAll(bool paused) {
	if (current == TRACK_NONE) return;
	sf::Music& m = tracks[current];
	if (paused && m.getStatus() == sf::SoundSource::Status::Playing) m.pause();
	else if (!paused && m.getStatus() == sf::SoundSource::Status::Paused) m.play();
}

void Audio::setTension(float f) {
	if (current == TRACK_GAME || current == TRACK_DREAD)
		tracks[current].setVolume(baseVolume[current] * (1.f - 0.4f * f));   // the heartbeat takes over
}
