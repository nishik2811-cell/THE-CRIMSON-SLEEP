#include "audio.h"
#include "globals.h"

Audio* g_audio = nullptr;

static const char* const SFX_FILES[SFX_COUNT] = {
	"jumpscare.ogg",                                         // SFX_JUMPSCARE
	"scream.ogg",                                            // SFX_SCREAM
	"sting_1.ogg sting_2.ogg",                               // SFX_STING
	"whisper_1.ogg whisper_2.ogg whisper_3.ogg",             // SFX_WHISPER
	"slam_1.ogg slam_2.ogg slam_3.ogg slam_4.ogg slam_metal.ogg",
	"hushed_breath.ogg scared_breath.ogg",                   // SFX_BREATH
	"slam_1.ogg slam_2.ogg slam_3.ogg",                      // SFX_CRACK (played low)
	"moan_1.ogg moan_2.ogg moan_3.ogg",                      // SFX_MOAN
	"glass_shatter_1.ogg glass_shatter_2.ogg glass_shatter_3.ogg",
	"heart_slow.ogg", "heart_fast.ogg",
	"hurt.ogg", "gasp.ogg", "death.ogg",
	"door_open_1.ogg door_open_2.ogg", "door_locked_1.ogg door_locked_2.ogg", "unlock.ogg", "keys.ogg",
	"pickup_1.ogg pickup_2.ogg", "paper_1.ogg paper_2.ogg", "lever_1.ogg lever_2.ogg", "memory_chime.ogg", "throw.ogg",
	"want_appear.ogg", "want_1.ogg want_2.ogg want_3.ogg",   // the Bride
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
		std::string list = SFX_FILES[s], name;
		for (size_t i = 0; i <= list.size(); ++i) {
			if (i == list.size() || list[i] == ' ') {
				if (!name.empty() && takes[s] < MAX_TAKES && buffers[s][takes[s]].loadFromFile(asset("audio/" + name))) takes[s]++;
				name.clear();
			}
			else name += list[i];
		}
	}
	struct { Track t; const char* file; float vol; bool loop; } list[] = {
		{ TRACK_TITLE,       "title_music.ogg",    80, true },
		{ TRACK_CHASE,       "chase_music.ogg",    85, true },
		{ TRACK_DEATH,       "ending_waking.ogg",  80, true },
		{ TRACK_END_TRUE,    "ending_morning.ogg", 85, true },
		{ TRACK_END_OTHER,   "memory_music.ogg",   85, true },
		{ TRACK_END_TRAPPED, "ending_waking.ogg",  85, true },
	};
	if (theme.openFromFile(asset("audio/rank_music.ogg"))) { theme.setLooping(true); theme.setVolume(60); }
	for (auto& e : list) {
		if (tracks[e.t].openFromFile(asset(std::string("audio/") + e.file))) { tracks[e.t].setLooping(e.loop); tracks[e.t].setVolume(e.vol); }
		trackVolume[e.t] = e.vol;
	}
}

void Audio::play(Sfx s, float volume, float pitch) {
	if (takes[s] == 0) return;
	std::optional<sf::Sound>& ch = channels[nextChannel];
	nextChannel = (nextChannel + 1) % CHANNELS;
	if (ch) ch->stop();
	ch.emplace(buffers[s][std::rand() % takes[s]]);
	ch->setVolume(volume);
	ch->setPitch(pitch * (0.95f + 0.1f * (std::rand() / (float)RAND_MAX)));   // tiny variation
	ch->play();
}

void Audio::startRoom() {
	if (theme.getStatus() != sf::SoundSource::Status::Playing) theme.play();
	theme.setVolume(60 * (1.f - 0.4f * tension));
	roomAmb.setVolume(40 * (1.f - 0.4f * tension));
	if (roomAmb.getStatus() != sf::SoundSource::Status::Playing) roomAmb.play();
	if (!layerName.empty() && roomLayer.getStatus() != sf::SoundSource::Status::Playing) roomLayer.play();
}
void Audio::stopRoom() { theme.pause(); roomAmb.pause(); roomLayer.pause(); }

void Audio::setRoom(const char* amb, const char* layer) {
	if (ambName != amb) {
		ambName = amb;
		roomAmb.stop();
		if (roomAmb.openFromFile(asset(std::string("audio/") + amb))) roomAmb.setLooping(true);
	}
	if (layerName != layer) {
		layerName = layer;
		roomLayer.stop();
		if (*layer && roomLayer.openFromFile(asset(std::string("audio/") + layer))) { roomLayer.setLooping(true); roomLayer.setVolume(30); }
	}
	if (current == TRACK_GAME) startRoom();
}

void Audio::music(Track t) {
	if (t == current) return;
	if (current == TRACK_GAME) stopRoom();
	else if (current != TRACK_NONE) tracks[current].stop();
	current = t;
	if (t == TRACK_GAME) startRoom();
	else if (t != TRACK_NONE) { tracks[t].setVolume(trackVolume[t]); tracks[t].play(); }
	if (t == TRACK_TITLE) { theme.stop(); roomAmb.stop(); roomLayer.stop(); ambName.clear(); layerName.clear(); }
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
	if (current == TRACK_GAME) { theme.setVolume(60 * (1.f - 0.4f * f)); roomAmb.setVolume(40 * (1.f - 0.4f * f)); }
}
