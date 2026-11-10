#include "audio.h"
#include "globals.h"

static void openLoop(sf::Music& m, const char* file, float volume) {
	if (m.openFromFile(asset(std::string("audio/") + file))) {
		m.setLooping(true);
		m.setVolume(volume);
	}
}

Audio::Audio() {
	openLoop(title, "title_piano.ogg", 70);
	openLoop(ambient, "ambient_musicbox.ogg", 32);
	openLoop(drone, "drone.ogg", 45);
	openLoop(ending, "ending_musicbox.ogg", 70);
	const char* files[SFX_COUNT] = { "jumpscare.ogg", "scream.ogg", "breath.ogg", "bells.ogg", "stinger.ogg", "whisper.ogg", "slam.ogg", "heartbeat.ogg" };
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
	current = t;
	title.stop(); ambient.stop(); drone.stop(); ending.stop();
	if (t == TRACK_TITLE) { title.play(); drone.setVolume(25); drone.play(); }
	if (t == TRACK_GAME) { ambient.play(); drone.setVolume(45); drone.play(); }
	if (t == TRACK_ENDING) ending.play();
}

void Audio::pauseAll(bool paused) {
	sf::Music* all[] = { &title, &ambient, &drone, &ending };
	for (sf::Music* m : all) {
		if (paused && m->getStatus() == sf::SoundSource::Status::Playing) m->pause();
		else if (!paused && m->getStatus() == sf::SoundSource::Status::Paused) m->play();
	}
}

void Audio::setTension(float f) {
	if (current == TRACK_GAME) {
		drone.setVolume(35 + 55 * f);
		drone.setPitch(1.f - 0.12f * f);
		ambient.setVolume(32 - 20 * f);
	}
}
