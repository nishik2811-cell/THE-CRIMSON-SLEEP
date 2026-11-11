#include "audio.h"
#include "globals.h"

Audio* g_audio = nullptr;

static const char* const SFX_FILES[SFX_COUNT] = {
	"jumpscare.ogg",                                         // SFX_JUMPSCARE
	"scream.ogg",                                            // SFX_SCREAM
	"sting_1.ogg sting_2.ogg",                               // SFX_STING
	"whisper_1.ogg whisper_2.ogg whisper_3.ogg",             // SFX_WHISPER
	"slam_1.ogg slam_2.ogg slam_3.ogg slam_4.ogg slam_metal.ogg",
	"whisper_1.ogg",                                         // SFX_BREATH (a soft whisper - no breathing)
	"slam_1.ogg slam_2.ogg slam_3.ogg",                      // SFX_CRACK (played low)
	"moan_1.ogg moan_2.ogg moan_3.ogg",                      // SFX_MOAN
	"glass_shatter_1.ogg glass_shatter_2.ogg glass_shatter_3.ogg",
	"heart_slow.ogg", "heart_fast.ogg",
	"hurt.ogg", "gasp.ogg", "death.ogg",
	"door_open_1.ogg door_open_2.ogg", "door_locked_1.ogg door_locked_2.ogg", "unlock.ogg", "keys.ogg",
	"pickup_1.ogg pickup_2.ogg", "paper_1.ogg paper_2.ogg", "lever_1.ogg lever_2.ogg", "memory_chime.ogg", "throw.ogg",
	"sting_1.ogg sting_2.ogg", "moan_1.ogg moan_2.ogg moan_3.ogg",   // the Bride: appears / weeps (moans pitched up)
	"voice_1.ogg voice_2.ogg voice_3.ogg voice_4.ogg", "drowned_lunge.ogg",   // the crawler
	"step_carpet_1.ogg step_carpet_2.ogg step_carpet_3.ogg step_carpet_4.ogg",
	"step_wood_1.ogg step_wood_2.ogg step_wood_3.ogg step_wood_4.ogg step_wood_5.ogg step_wood_6.ogg",
	"step_wood_1.ogg step_wood_2.ogg step_wood_3.ogg",      // (was creaky boards)
	"step_stone_1.ogg step_stone_2.ogg step_stone_3.ogg",
	"step_water_1.ogg step_water_2.ogg step_water_3.ogg step_water_4.ogg",
	"ui_move.ogg", "ui_click.ogg",
};

Audio::Audio() {
	g_audio = this;
	for (int s = 0; s < SFX_COUNT; ++s) {
		Str list = SFX_FILES[s], name;
		for (int i = 0; i <= list.size(); ++i) {
			if (i == list.size() || list[i] == ' ') {
				if (!name.empty() && takes[s] < MAX_TAKES && buffers[s][takes[s]].loadFromFile(asset("audio/" + name))) takes[s]++;
				name.clear();
			}
			else name += list[i];
		}
	}
	struct { Track t; const char* file; float vol; bool loop; } list[] = {
		{ TRACK_TITLE,       "title_music.ogg",    80, true },
		{ TRACK_CHASE,       "chase.ogg",          85, true },
		{ TRACK_DEATH,       "music_wrong.ogg",    80, true },
		{ TRACK_END_TRUE,    "ending_true.ogg",    85, true },
		{ TRACK_END_OTHER,   "rank_music.ogg",     85, true },
		{ TRACK_END_TRAPPED, "music_wrong.ogg",    85, true },
	};
	const char* themeFiles[3] = { "theme_calm.ogg", "theme_dread.ogg", "theme_nursery.ogg" };
	for (int i = 0; i < 3; ++i)
		if (themes[i].openFromFile(asset(Str("audio/") + themeFiles[i]))) { themes[i].setLooping(true); themes[i].setVolume(60); }
	if (pauseMusic.openFromFile(asset("audio/pause_music.ogg"))) { pauseMusic.setLooping(true); pauseMusic.setVolume(65); }
	for (auto& e : list) {
		if (tracks[e.t].openFromFile(asset(Str("audio/") + e.file))) { tracks[e.t].setLooping(e.loop); tracks[e.t].setVolume(e.vol); }
		trackVolume[e.t] = e.vol;
	}
}

void Audio::play(Sfx s, float volume, float pitch) {
	if (takes[s] == 0) return;
	sf::Sound*& ch = channels[nextChannel];
	nextChannel = (nextChannel + 1) % CHANNELS;
	if (ch) { ch->stop(); delete ch; }
	ch = new sf::Sound(buffers[s][std::rand() % takes[s]]);
	ch->setVolume(volume);
	ch->setPitch(pitch * (0.95f + 0.1f * (std::rand() / (float)RAND_MAX)));   // tiny variation
	ch->play();
}

void Audio::startRoom() {
	for (int i = 0; i < 3; ++i)
		if (i != themeIdx && themes[i].getStatus() == sf::SoundSource::Status::Playing) themes[i].pause();
	if (theme().getStatus() != sf::SoundSource::Status::Playing) theme().play();
	theme().setVolume(60 * (1.f - 0.4f * tension));
	roomAmb.setVolume(40 * (1.f - 0.4f * tension));
	if (roomAmb.getStatus() != sf::SoundSource::Status::Playing) roomAmb.play();
	if (!layerName.empty() && roomLayer.getStatus() != sf::SoundSource::Status::Playing) roomLayer.play();
}
void Audio::stopRoom() { for (sf::Music& m : themes) m.pause(); roomAmb.pause(); roomLayer.pause(); }

void Audio::setRoom(const char* amb, const char* layer, int theme) {
	themeIdx = theme < 0 || theme > 2 ? 0 : theme;
	if (ambName != amb) {
		ambName = amb;
		roomAmb.stop();
		if (roomAmb.openFromFile(asset(Str("audio/") + amb))) roomAmb.setLooping(true);
	}
	if (layerName != layer) {
		layerName = layer;
		roomLayer.stop();
		if (*layer && roomLayer.openFromFile(asset(Str("audio/") + layer))) { roomLayer.setLooping(true); roomLayer.setVolume(30); }
	}
	if (current == TRACK_GAME) startRoom();
}

void Audio::music(Track t) {
	if (inPause && t != current) { pauseMusic.stop(); inPause = false; }   // e.g. pause menu -> title
	if (t == current) return;
	if (current == TRACK_GAME) stopRoom();
	else if (current != TRACK_NONE) tracks[current].stop();
	current = t;
	if (t == TRACK_GAME) startRoom();
	else if (t != TRACK_NONE) { tracks[t].setVolume(trackVolume[t]); tracks[t].play(); }
	if (t == TRACK_TITLE) { for (sf::Music& m : themes) m.stop(); roomAmb.stop(); roomLayer.stop(); ambName.clear(); layerName.clear(); }
}

void Audio::pauseAll(bool paused) {
	if (current == TRACK_GAME) { if (paused) stopRoom(); else startRoom(); return; }
	if (current == TRACK_NONE) return;
	sf::Music& m = tracks[current];
	if (paused && m.getStatus() == sf::SoundSource::Status::Playing) m.pause();
	else if (!paused && m.getStatus() == sf::SoundSource::Status::Paused) m.play();
}

void Audio::setTension(float f) {
	tension = f;
	if (current == TRACK_GAME) { theme().setVolume(60 * (1.f - 0.4f * f)); roomAmb.setVolume(40 * (1.f - 0.4f * f)); }
}

void Audio::setPaused(bool paused) {
	if (paused == inPause) return;
	inPause = paused;
	if (paused) {
		pauseAll(true);                 // hold the game music where it is
		pauseMusic.play();
	}
	else {
		pauseMusic.pause();             // the pause track remembers its place too
		pauseAll(false);
	}
}

void Audio::toggleMute() {
	muted = !muted;
	sf::Listener::setGlobalVolume(muted ? 0.f : 100.f);
}
