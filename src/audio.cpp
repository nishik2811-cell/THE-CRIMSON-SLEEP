#include "audio.h"
#include "globals.h"

Audio::Audio() {
	title.openFromFile(asset("audio/title_piano.ogg"));     title.setLoop(true);   title.setVolume(70);
	ambient.openFromFile(asset("audio/ambient_musicbox.ogg")); ambient.setLoop(true); ambient.setVolume(32);
	drone.openFromFile(asset("audio/drone.ogg"));           drone.setLoop(true);   drone.setVolume(45);
	ending.openFromFile(asset("audio/ending_musicbox.ogg"));  ending.setLoop(true);  ending.setVolume(70);
	const char* files[SFX_COUNT] = { "jumpscare.ogg", "scream.ogg", "breath.ogg", "bells.ogg", "stinger.ogg", "whisper.ogg", "slam.ogg", "heartbeat.ogg" };
	for (int i = 0; i < SFX_COUNT; ++i) buffers[i].loadFromFile(asset(std::string("audio/") + files[i]));
}

void Audio::play(Sfx s, float volume, float pitch) {
	sf::Sound& ch = channels[nextChannel];
	nextChannel = (nextChannel + 1) % CHANNELS;
	ch.stop();
	ch.setBuffer(buffers[s]);
	ch.setVolume(volume);
	ch.setPitch(pitch);
	ch.play();
}

void Audio::music(Track t) {
	if (t == current) return;
	current = t;
	title.stop(); ambient.stop(); drone.stop(); ending.stop();
	if (t == TRACK_TITLE) { title.play(); drone.setVolume(25); drone.play(); }
	if (t == TRACK_GAME) { ambient.play(); drone.setVolume(45); drone.play(); }
	if (t == TRACK_ENDING) ending.play();
}

void Audio::pauseAll(bool paused) {
	sf::Music* all[] = { &title, &ambient, &drone, &ending };
	for (sf::Music* m : all) {
		if (paused && m->getStatus() == sf::Music::Playing) m->pause();
		else if (!paused && m->getStatus() == sf::Music::Paused) m->play();
	}
}

void Audio::setTension(float f) {
	if (current == TRACK_GAME) {
		drone.setVolume(35 + 55 * f);
		drone.setPitch(1.f - 0.12f * f);
		ambient.setVolume(32 - 20 * f);
	}
}
