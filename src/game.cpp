#include "game.h"
#include "ds/MinMax.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>

static float frand(float a, float b) { return a + (b - a) * (std::rand() / (float)RAND_MAX); }
static float dist(sf::Vector2f a, sf::Vector2f b) { float x = a.x - b.x, y = a.y - b.y; return std::sqrt(x * x + y * y); }

const TipInfo TIPS[TIP_COUNT] = {
	{ "Restless Spirits",
	  "They walk the same path, over and over, the way they did in 1953.\n"
	  "- Watch a patrol once before you cross it.\n"
	  "- A lantern FLARE (F) freezes them for a few seconds.\n"
	  "- Contact drains your blood and spikes your fear." },
	{ "Samara (The Cursed Maiden)",
	  "She drifts through walls, hunting by the sound of your steps.\n"
	  "- STAND STILL and she loses your trail.\n"
	  "- FLARE your lantern (F) when she is close to banish her.\n"
	  "- Her locket lies somewhere below the chapel. Return it to her\n"
	  "  and she will never harm you again." },
	{ "Carrie (The Blood Oracle)",
	  "Drenched in prom blood, her telekinesis strikes when unseen.\n"
	  "- FACE HER directly and she cannot move.\n"
	  "- Walk backwards toward the door with eyes locked on Carrie.\n"
	  "- A FLARE (F) disrupts her psychic grip and drives her back." },
	{ "The Devil",
	  "He waits in the crypt. Only the Blessed Sword can end him.\n"
	  "- SHIFT swings the sword. Six true blows and he falls.\n"
	  "- His claws tear at you - strike, then step back.\n"
	  "- A FLARE (F) stuns him: that's your moment to strike." },
	{ "Spikes, Fire and Graves",
	  "The house itself wants you.\n"
	  "- Spikes and flames rise and fall - time your steps.\n"
	  "- Some rooms have a lever that silences the spikes." },
	{ "Fear",
	  "Fear rises in the dark, near the dead, and with every scare.\n"
	  "- At 100 fear your mind breaks.\n"
	  "- Keep the lantern lit and rest in quiet rooms to calm down.\n"
	  "- Flaring the lantern steadies your nerves." },
	{ "The Weavers (Spiders)",
	  "Too many legs, and they live in the dark between the lights.\n"
	  "- They skitter in bursts, then freeze. When you come close, they RUSH.\n"
	  "- STAND STILL, turn your lantern OFF (L) or HIDE in a wardrobe (E).\n"
	  "- When one is at your feet, STOMP (Q) - squashed for good.\n"
	  "- A FLARE (F) scares them back to their nest." },
};

// Per-room audio: ambience loop, optional extra loop, and footstep surface.
static const char* const ROOM_AMB[ROOM_COUNT] = { "", "amb_house.ogg", "amb_gallery.ogg", "amb_library.ogg", "drone_tension.ogg",
	"amb_crawl.ogg", "amb_crimson.ogg", "drone_dread.ogg", "amb_crimson.ogg", "amb_drowned.ogg", "drone_dread.ogg" };
static const char* const ROOM_LAYER[ROOM_COUNT] = { "", "", "wind_loop.ogg", "", "", "", "", "", "rumble_loop.ogg", "drip_loop.ogg", "water_loop.ogg" };
static const Sfx ROOM_STEP[ROOM_COUNT] = { SFX_STEP_CARPET, SFX_STEP_CARPET, SFX_STEP_WOOD, SFX_STEP_WOOD, SFX_STEP_WOOD,
	SFX_STEP_CARPET, SFX_STEP_STONE, SFX_STEP_STONE, SFX_STEP_STONE, SFX_STEP_WATER, SFX_STEP_STONE };

// ------------------------------------------------------------------ setup
Game::Game(Audio& a) : audio(a) {
	g_lastLookup = -1; g_consoleScroll = 0;
	if (deadTex.loadFromFile(asset("graphics/spider_dead.png"))) deadSprite.setTexture(deadTex, true);
	// The card's blood: find where each thin strip of the drip image ends, so the
	// strips can be revealed downwards one by one - the blood runs down the card.
	{
		sf::Image bi;
		if (bi.loadFromFile(asset("graphics/blood_drip.png"))) {
			(void)bloodTex.loadFromImage(bi);
			bloodSprite.setTexture(bloodTex, true);
			for (unsigned s = 0; s < 128 && s * 3 < bi.getSize().x; ++s) {
				int last = 0;
				for (unsigned yy = 0; yy < bi.getSize().y; ++yy)
					for (unsigned xx = s * 3; xx < s * 3 + 3 && xx < bi.getSize().x; ++xx)
						if (bi.getPixel({ xx, yy }).a > 60) last = (int)yy;
				bloodTip[s] = last;
			}
		}
	}
	deadSprite.setOrigin({ 32.f, 32.f }); deadSprite.setScale({ 0.6f, 0.6f });
	(void)tileset.loadFromFile(asset("graphics/texture.png"));
	for (int r = 1; r < ROOM_COUNT; ++r) maps[r].initializeMap(ROOMS[r].mapFile, tileset);
	buildHouse(house);
	buildItemDatabase(itemDB);
	(void)fontTitle.openFromFile(asset("fonts/GrenzeGotisch.ttf"));
	(void)fontSerif.openFromFile(asset("fonts/CormorantGaramond.ttf"));
	(void)fontItalic.openFromFile(asset("fonts/CormorantGaramond-Italic.ttf"));
	(void)fontMono.openFromFile(asset("fonts/DejaVuSans.ttf"));
	(void)fontBlood.openFromFile(asset("fonts/Nosifer.ttf"));   // dripping letters for the blood writing
	if (jumpTex.loadFromFile(asset("graphics/jumpscare.png"))) jumpSprite.setTexture(jumpTex, true);
	if (decalTex.loadFromFile(asset("graphics/blood_decals.png"))) decalSprite.setTexture(decalTex, true);
	if (alfredDeadTex.loadFromFile(asset("graphics/servent_dead.png"))) alfredDeadSprite.setTexture(alfredDeadTex, true);
	alfredDeadSprite.setOrigin({ 0.f, 24.f });
	jumpSprite.setOrigin({ (float)(jumpTex.getSize().x / 2.f), (float)(jumpTex.getSize().y / 2.f) });
	npc.setPosition({ (float)(546), (float)(236) });
	npc.initialize();
	enterRoom(ENTRANCE_HALL, -1, true);
	say("You wake on the cold floor of the Entrance Hall.\n"
		"The house has been sleeping in crimson since June 26, 1953.\n"
		"Find a way to end the nightmare.\n\n"
		"(Move: Arrows/WASD   Interact: E/Enter   Dismiss: Space)");
}

void Game::devStartAt(int r) {
	// Give everything a player would have collected to reach room r.
	const int need[ROOM_COUNT][4] = { {0}, {0}, { LANTERN }, { LANTERN }, { LANTERN, RUSTY_KEY }, { LANTERN, BRASS_KEY },
		{ LANTERN, CHAPEL_KEY }, { LANTERN, IRON_KEY }, { LANTERN, SWORD }, { LANTERN, HYMN_PAGE }, { LANTERN, NEWSPAPER } };
	for (int i = 0; i < 4; ++i) if (need[r][i]) {
		player.give(need[r][i]);
		if (ItemRecord* rec = itemDB.search(need[r][i])) { rec->collected = true; rec->discovered = true; }
		g_lastLookup = need[r][i];
	}
	if (r == HIDDEN_TUNNEL || r == OSSUARY) house.reveal(CHAPEL, HIDDEN_TUNNEL);
	// Open every door whose key the dev start handed over.
	for (int a = 1; a < ROOM_COUNT; ++a)
		for (const Edge* e = house.neighbours(a); e; e = e->next)
			if (e->locked && e->to != OUTSIDE && player.has(e->requiredItem)) house.unlock(a, e->to);
	dialogue = "";
	tipSeen[TIP_FEAR] = true;
	enterRoom(r, -1, true);
}

void Game::devSpawnBride() {
	bride.reset(true, 0.01f);
	bride.update(0.02f, pp(), true);
	bride.pos = pp() + sf::Vector2f(70, -10);
	bride.state = Bride::HUNT; bride.alpha = 210; bride.timer = 15;
}

// ------------------------------------------------------------- room flow
sf::Vector2i Game::spawnNearDoor(int inRoom, int fromRoom) {
	const Edge* e = house.edgeTo(inRoom, fromRoom);
	int c, r;
	if (e && maps[inRoom].map.find(3 + e->doorNo, &c, &r)) {
		const int dx[4] = { 0, 0, -1, 1 }, dy[4] = { 1, -1, 0, 0 };
		for (int k = 0; k < 4; ++k) {
			int c1 = c + dx[k], r1 = r + dy[k];
			if (maps[inRoom].map.inside(c1, r1) && !maps[inRoom].map.solid(c1, r1) && maps[inRoom].map.at(c1, r1).x != 0)
				return sf::Vector2i(c1, r1);
		}
	}
	return sf::Vector2i(ROOMS[inRoom].spawnX, ROOMS[inRoom].spawnY);
}

void Game::placePlayer(sf::Vector2i t) { player.setPosition({ (float)(t.x * 32 + 4), (float)(t.y * 32 + 4) }); }

void Game::enterRoom(int to, int from, bool pushHistory) {
	// A spider that was rushing you doesn't stop at the door.
	bool chased = false;
	for (int i = 0; i < spiderCount; ++i)
		if (spiders[i].active && (spiders[i].state == Spider::CHASE || spiders[i].state == Spider::ATTACK)) chased = true;
	room = to;
	current_map = &maps[room].map;
	entryTile = from < 0 ? sf::Vector2i(ROOMS[room].spawnX, ROOMS[room].spawnY) : spawnNearDoor(room, from);
	placePlayer(entryTile);
	if (pushHistory) player.history.push(room);
	if (!roomVisited[room]) { roomVisited[room] = true; runStats.roomsSeen++; }
	roomTime = 0; digT = 0; trailLen = 0;
	spawnEnemies();
	spiderFollowT = chased ? 1.2f : -1.f;    // it scuttles in behind you a moment later
	audio.setRoom(ROOM_AMB[room], ROOM_LAYER[room], room == NURSERY ? Audio::THEME_NURSERY : ROOMS[room].danger >= 7 ? Audio::THEME_DREAD : Audio::THEME_CALM);

	// Schedule this visit's haunted events (FIFO, in chronological order).
	events.clear();
	int n = 1 + ROOMS[room].danger / 3;
	float t = frand(5, 10);
	for (int i = 0; i < n; ++i) {
		events.enqueue(HauntEvent{ t, std::rand() % EV_KIND_COUNT });
		t += frand(8, 18);
	}

	switch (room) {
	case GRAND_CORRIDOR: if (!roomVisited[LIBRARY]) say("A long corridor. Portraits with scratched-out eyes.\nAlfred was right to be afraid."); break;
	case NURSERY: say("A music box plays by itself.\nI must not come in contact with whatever lives here.\nSpikes ring the chest - there must be a lever somewhere."); break;
	case CRIMSON_CRYPT: say("The Devil is here. The EMF meter screams.\nHe can only be ended by the Blessed Sword:\nSHIFT to strike - six blows. A flare (F) stuns him."); break;
	case HIDDEN_TUNNEL: say("Wet stone. Scratch marks. Someone was walled in here."); break;
	case OSSUARY: say("Bones, stacked like firewood.\nAt the far end, a skeleton in a wedding dress."); break;
	default: break;
	}
}

void Game::spawnEnemies() {
	easyCount = mediumCount = 0; bossActive = false;
	if (room == NURSERY) {
		// Gentler than the original six: no stacked pair, nobody patrolling the
		// chest's column, and they drift a little slower here.
		sf::Vector2i e[2] = { {196, 452}, {196, 324} }, m[1] = { {676, 164} };
		for (int i = 0; i < 2; ++i) { ghostEasy[i].setPosition({ (float)(e[i].x), (float)(e[i].y) }); ghostEasy[i].initialize(); ghostEasy[i].velocity = 0.4f; }
		ghostMedium[0].setPosition({ (float)(m[0].x), (float)(m[0].y) }); ghostMedium[0].initialize(); ghostMedium[0].velocity = 0.45f;
		easyCount = 2; mediumCount = 1;
	}
	else if (room == CHAPEL) {
		sf::Vector2i e[3] = { {356, 388}, {324, 516}, {420, 260} }, m[2] = { {740, 132}, {388, 132} };
		for (int i = 0; i < 3; ++i) { ghostEasy[i].setPosition({ (float)(e[i].x), (float)(e[i].y) }); ghostEasy[i].initialize(); }
		for (int i = 0; i < 2; ++i) { ghostMedium[i].setPosition({ (float)(m[i].x), (float)(m[i].y) }); ghostMedium[i].initialize(); }
		easyCount = 3; mediumCount = 2;
	}
	else if (room == CRIMSON_CRYPT && bossHP > 0) { boss.setPosition({ (float)(838), (float)(228) }); bossActive = true; }

	bride.reset(ROOMS[room].wraith, frand(10, 18));
	bride.pacified = player.has(CRIMSON_LOCKET);
	bool crawl = room == NURSERY || room == HIDDEN_TUNNEL || room == OSSUARY || room == COLLAPSED_BEDROOM;
	sf::Vector2f spot = room == NURSERY ? sf::Vector2f(780, 520) : room == HIDDEN_TUNNEL ? sf::Vector2f(700, 180) :
		room == OSSUARY ? sf::Vector2f(250, 330) : sf::Vector2f(620, 560);
	child.reset(crawl, spot);
	child.wake = room == NURSERY ? 25.f : 0.f;      // in the Nursery it waits a while before hunting
	child.speed = room == NURSERY ? 85.f : 115.f;
	childRoom = crawl; childBanishT = 0;
	// Spiders: dropped on random floor tiles well away from where you come in.
	// 11 in the house: two in the Hidden Tunnel, one in every other room.
	spiderCount = room == HIDDEN_TUNNEL ? 2 : (room >= ENTRANCE_HALL && room < ROOM_COUNT) ? 1 : 0;
	spiderCount = maxOf(0, spiderCount - corpseCount[room]);        // squashed ones don't come back
	for (int i = 0; i < 3; ++i) spiders[i].reset(false, {});
	for (int i = 0; i < spiderCount; ++i) {
		const TileGrid& m = maps[room].map;
		sf::Vector2f at = pp() + sf::Vector2f(12, 12);
		for (int tries = 0; tries < 60; ++tries) {
			int col = std::rand() % GRID_W, row = std::rand() % GRID_H;
			sf::Vector2i t = m.at(col, row);
			if (m.solid(col, row) || (t.x == 0 && t.y == 0)) continue;
			sf::Vector2f p(col * 32.f + 16, row * 32.f + 16);
			if (dist(p, pp()) < 200) continue;
			at = p; break;
		}
		spiders[i].reset(true, at);
	}
	ghostStun = bossStun = 0;
	chaseHold = 0;
}

void Game::retryRoom() {
	player.health = 100; player.fear = 35; player.invulnerable = 2.f; player.frozen = false;
	jumpscare = 0; hurtFlash = 0;
	if (room == CRIMSON_CRYPT) { digCount = digCount >= 4 ? 4 : 0; }
	placePlayer(entryTile);
	spawnEnemies();
	dialogue = "";
	say("You wake again, gasping. The house lets you try once more.");
}

// ------------------------------------------------------------ main update
void Game::update(sf::RenderWindow* window, float dt) {
	lastDt = dt;
	updateDoor(dt);
	if (demo && doorT < 0) demoUpdate(dt);
	bool overlay = tipOpen >= 0 || journalPage >= 0;
	if (!overlay) {
		runStats.time += dt;
		roomTime += dt;
		if (player.invulnerable > 0) player.invulnerable -= dt;
		if (flareCooldown > 0) flareCooldown -= dt;
		if (tipCardT > 0) tipCardT -= dt;
		if (stompT > 0) stompT -= dt;
		if (swingT > 0) swingT -= dt;
		if (bossHitT > 0) bossHitT -= dt;
		if (swingHitT > 0) { swingHitT -= dt; if (swingHitT <= 0) swordLand(); }
		if (stompHitT > 0) { stompHitT -= dt; if (stompHitT <= 0) stompLand(); }
		if (stompFxT > 0) stompFxT -= dt;
		if (flare > 0) flare -= dt * 2.f;
		if (shake > 0) shake -= dt * 2.5f;
		if (hurtFlash > 0) hurtFlash -= dt * 1.5f;
		if (toastT > 0) toastT -= dt;
		if (eventCaptionT > 0) eventCaptionT -= dt;
		if (wallT > 0) wallT -= dt;
		if (ghostStun > 0) ghostStun -= dt;
		if (bossStun > 0) bossStun -= dt;
		if (childBanishT > 0) { childBanishT -= dt; if (childBanishT <= 0 && childRoom) child.reset(true, child.home); }
		if (jumpscare > 0) { jumpscare -= dt; player.frozen = jumpscare > 0.25f; }
		player.light.flicker *= 0.92f;
	}
	player.frozen = overlay || jumpscare > 0.25f || hidden || doorT >= 0 || (arriveT >= 0 && arriveT < WALK_OUT);

	window->setView(player.view);   // the room always follows the camera
	// Camera shake.
	if (shake > 0) {
		sf::View v = player.view;
		v.move({ (float)(frand(-4, 4) * shake), (float)(frand(-4, 4) * shake) });
		window->setView(v);
	}

	maps[room].drawMap(window);
	if (doorT >= 0 && doorT < DOOR_OPEN + DOOR_FADE) drawDoorTile(window, doorTile, minOf(1.f, doorT / DOOR_OPEN), 0);   // swinging open
	if (arriveT >= 0 && arriveT < WALK_OUT + DOOR_IN)                                                                  // open while you step out, then shuts behind you
		drawDoorTile(window, arriveTile, arriveT < WALK_OUT ? 1.f : 1.f - (arriveT - WALK_OUT) / DOOR_IN, 0, false);
	if (rattleT > 0) drawDoorTile(window, rattleTile, 0.04f * std::fabs(std::sin(rattleT * 45.f)), std::sin(rattleT * 90.f) * 1.4f);   // locked: it rattles in its frame

	// --- characters drawn under the darkness ---
	bool talkedToAlfred = updateAlfred(dt, window, overlay);
	drawDeadAlfred(window, dt, overlay);
	for (int i = 0; i < easyCount; ++i) {
		if (overlay || ghostStun > 0) { ghostEasy[i].characterImage.setColor(ghostStun > 0 ? sf::Color(255, 140, 140) : sf::Color::White); window->draw(ghostEasy[i].characterImage); }
		else { ghostEasy[i].characterImage.setColor(sf::Color::White); ghostEasy[i].movement(&player, window); }
	}
	for (int i = 0; i < mediumCount; ++i) {
		if (overlay || ghostStun > 0) { ghostMedium[i].characterImage.setColor(ghostStun > 0 ? sf::Color(255, 140, 140) : sf::Color::White); window->draw(ghostMedium[i].characterImage); }
		else { ghostMedium[i].characterImage.setColor(sf::Color::White); ghostMedium[i].movement(&player, window); }
	}
	if (bossActive) {
		boss.characterImage.setColor(bossHitT > 0 ? sf::Color(255, 90, 90) : sf::Color::White);   // flashes red when struck
		if (overlay || bossStun > 0) { window->draw(boss.characterImage); boss.beep_sound.stop(); }
		else boss.movement(&player, window);
	}
	else if (bossDeathT >= 0) {                      // the Devil collapses: sinks, darkens, fades
		float k = minOf(1.f, bossDeathT / 2.2f);
		boss.characterImage.setColor(sf::Color(255, (std::uint8_t)(120 * (1 - k)), (std::uint8_t)(120 * (1 - k)), (std::uint8_t)(255 * (1 - k))));
		window->draw(boss.characterImage);
		if (!overlay) bossDeathT += dt;
		if (bossDeathT > 2.6f) { bossDeathT = -1; player.end_sound.play(); finish(player.has(CRIMSON_LOCKET) ? END_TRUE : END_ESCAPE); return; }
	}
	if (!overlay) child.update(dt, pp(), player.source.y);
	child.draw(window);
	if (spiderFollowT > 0 && !overlay) {
		spiderFollowT -= dt;
		if (spiderFollowT <= 0 && spiderCount < 3) {
			Spider& s = spiders[spiderCount++];
			s.reset(true, sf::Vector2f(entryTile.x * 32.f + 16, entryTile.y * 32.f + 16));   // in through the door you used
			s.startChase(pp() + sf::Vector2f(12, 12));   // it already knows where you are
			audio.play(SFX_STING, 60, 1.6f);
			showToast("Something followed you through the door...");
		}
	}
	// Squashed spiders lie where they died.
	for (int i = 0; i < corpseCount[room]; ++i) { deadSprite.setPosition(corpses[room][i]); window->draw(deadSprite); }
	for (int i = 0; i < spiderCount; ++i) {
		Spider& s = spiders[i];
		sf::Vector2f me = pp() + sf::Vector2f(12, 12);
		if (!overlay) {
			if (player.isMoving() && !hidden) s.hear(me, 140.f);        // footsteps carry; standing still is silent
			s.update(dt, me, !hidden, lanternOn ? 260.f : 90.f, maps[room].map);   // lantern off: it only sees you up close
			if (s.active && !hidden && dist(s.pos, me) < 22) {
				if (s.bite <= 0 && s.dangerous()) {
					audio.play(SFX_STING, 70, 1.4f);
					hurt(8, 10, "A spider skitters up your arm and BITES!", TIP_SPIDERS, false);
					s.bitten();
				}
			}
		}
		s.draw(window);
	}

	// --- player + lantern darkness ---
	if (shake > 0) player.view.move({ (float)(0), (float)(0) });
	player.light.dim = lanternOn ? 1.f : 0.42f;
	player.movement(window);
	{                                              // stomp range: a faint ring at your feet when a spider is near
		sf::Vector2f me = pp() + sf::Vector2f(12, 12);
		float nearest = 1e9f;
		for (int i = 0; i < spiderCount; ++i) if (spiders[i].active) nearest = minOf(nearest, dist(spiders[i].pos, me));
		if (nearest < 140 && !hidden) {
			bool inside = nearest < STOMP_REACH;
			float k = 1.f - maxOf(0.f, (nearest - STOMP_REACH) / (140 - STOMP_REACH));
			sf::CircleShape ring(STOMP_REACH); ring.setOrigin({ STOMP_REACH, STOMP_REACH }); ring.setPosition(me);
			ring.setFillColor(inside ? sf::Color(200, 30, 50, 40) : sf::Color::Transparent);
			ring.setOutlineThickness(1.5f);
			ring.setOutlineColor(inside ? sf::Color(255, 70, 90, 220) : sf::Color(220, 200, 180, (std::uint8_t)(50 + 90 * k)));
			window->draw(ring);
		}
	}
	if (stompFxT > 0) {                            // dust ring kicked out by the stamp
		float k = 1.f - stompFxT / 0.35f;
		sf::CircleShape ring(6.f + 22.f * k); ring.setOrigin({ ring.getRadius(), ring.getRadius() });
		ring.setScale({ 1.f, 0.45f });
		ring.setPosition(pp() + sf::Vector2f(12.f, 22.f));
		ring.setFillColor(sf::Color::Transparent);
		ring.setOutlineThickness(2.f); ring.setOutlineColor(sf::Color(190, 165, 140, (std::uint8_t)(200 * (1 - k))));
		window->draw(ring);
	}
	if (shake > 0) {
		sf::View v = player.view; v.move({ (float)(frand(-4, 4) * shake), (float)(frand(-4, 4) * shake) }); window->setView(v);
	}

	// --- horrors that glow through the dark ---
	if (!overlay) {
		bride.update(dt, pp(), player.isMoving());
	}
	bride.draw(window);
	drawWorldOverlays(window);
	for (int si = 0; si < spiderCount; ++si) spiders[si].drawEyes(window, pp() + sf::Vector2f(12, 12));   // red glints in the dark
	for (int si = 0; si < spiderCount; ++si) {
		const Spider& s = spiders[si];
		// With the console open, show what the AI is planning: its path as dots on the floor.
		if (g_panelVP.size.x > 0 && s.active && s.pathLen > s.pathStep) {
			sf::Color pc = s.state == Spider::CHASE || s.state == Spider::ATTACK ? sf::Color(240, 60, 70, 170) :
			               s.state == Spider::INVESTIGATE ? sf::Color(240, 190, 90, 160) : s.state == Spider::RETURN ? sf::Color(120, 160, 230, 150) :
			               s.state == Spider::PATROL ? sf::Color(150, 140, 140, 120) : sf::Color(200, 150, 220, 140);
			for (int k = s.pathStep; k < s.pathLen; ++k) {
				sf::CircleShape dot(2.5f); dot.setOrigin({ 2.5f, 2.5f });
				dot.setPosition(sf::Vector2f((s.path[k] % GRID_W) * 32.f + 16, (s.path[k] / GRID_W) * 32.f + 16));
				dot.setFillColor(pc); window->draw(dot);
			}
		}
	}
   // hint trail, target marker, blood writing - drawn over the darkness so they can be followed

	if (!overlay) {
		// Story / interaction.
		if (key_hit(sf::Keyboard::Key::Space) && alfredT < 0) dialogue = "";   // during Alfred's talk Space = next line
		if (!talkedToAlfred && (key_hit(sf::Keyboard::Key::Enter) || key_hit(sf::Keyboard::Key::E))) interact();
		if (key_hit(sf::Keyboard::Key::B)) goBack();
		if (key_hit(sf::Keyboard::Key::F)) flareLantern();
		if (key_hit(sf::Keyboard::Key::Q)) stomp();
		if (key_hit(sf::Keyboard::Key::LShift) || key_hit(sf::Keyboard::Key::RShift)) swingSword();
		if (key_hit(sf::Keyboard::Key::L) && player.has(LANTERN)) {      // lantern on/off (stealth)
			lanternOn = !lanternOn;
			audio.play(SFX_UI_CLICK, 70);
			showToast(lanternOn ? "Lantern lit." : "Lantern out. You see less - and are seen less.");
		}
		if (key_hit(sf::Keyboard::Key::H)) { if (objectiveTime < 31) objectiveTime = 31; }
		if (key_hit(sf::Keyboard::Key::I)) showInventory = !showInventory;
		if (key_hit(sf::Keyboard::Key::J)) journalPage = 0;

		updateHazards();
		updateVoices(dt);
		updateDig(dt);
		updateEvents(dt);
		updateThreats(dt);
		updateHints(dt);
		if (bossActive && bossStun <= 0 && !hidden && player.invulnerable <= 0 && boss.isCollisionCharacter(&player)) {
			audio.play(SFX_SCREAM, 70, 0.6f);
			hurt(25, 15, "The Devil's claws tear into you!", TIP_DEVIL, false);
			knockBack(boss.characterImage.getPosition());
			if (game_state != ST_PLAYING) return;
		}
	}
	else {
		if (key_hit(sf::Keyboard::Key::Enter) || key_hit(sf::Keyboard::Key::Space) || key_hit(sf::Keyboard::Key::Escape) || key_hit(sf::Keyboard::Key::J)) {
			if (tipOpen >= 0) tipOpen = -1;
			else journalPage = -1;
		}
		if (journalPage >= 0 && (key_hit(sf::Keyboard::Key::Right) || key_hit(sf::Keyboard::Key::D))) journalPage = (journalPage + 1) % TIP_COUNT;
		if (journalPage >= 0 && (key_hit(sf::Keyboard::Key::Left) || key_hit(sf::Keyboard::Key::A))) journalPage = (journalPage + TIP_COUNT - 1) % TIP_COUNT;
	}

	drawHud(window);
	window->setView(player.view);   // keep world drawing aligned next frame
}

// ------------------------------------------------------------ interaction
void Game::changeFacingTile(sf::Vector2i v) {
	sf::Vector2i f = player.col.facingTile(player.source.y);
	maps[room].map.set(f.x, f.y, v);
}

void Game::pickUp(int item, const Str& msg, bool clearTile) {
	bool isKey = item == RUSTY_KEY || item == BRASS_KEY || item == CHAPEL_KEY || item == IRON_KEY;
	audio.play(item == CRIMSON_LOCKET ? SFX_CHIME : ITEMS[item].isClue ? SFX_PAPER : isKey ? SFX_KEYS : SFX_PICKUP, 85);
	ItemRecord* rec = itemDB.search(item);
	g_lastLookup = item;
	if (rec && !rec->collected) { rec->collected = true; runStats.itemsFound++; }
	if (ITEMS[item].isClue) { if (rec) rec->discovered = true; }
	player.give(item);
	showToast(Str("+ ") + ITEMS[item].name);
	say(msg);
	if (clearTile) changeFacingTile(sf::Vector2i(-1, -1));
}

void Game::interact() {
	int id = player.col.isInteractionMap(pp(), player.source.y);
	if (id < 0) return;
	switch (id) {
	case 0: { // drawer: each one holds exactly one thing - a story item, a health kit, or a scare
		sf::Vector2i ft = player.col.facingTile(player.source.y);
		bool& done = searched[room][ft.y * GRID_W + ft.x];
		bool first = !done;
		done = true;
		if (room == LIBRARY && !collected(RUSTY_KEY)) pickUp(RUSTY_KEY, "Drawer:\n\tA rusty key, wrapped in a child's ribbon.", false);
		else if (room == GRAND_CORRIDOR && !collected(NEWSPAPER))
			pickUp(NEWSPAPER, "A yellowed newspaper - June 26, 1953:\n\tA man killed ten innocent people, women and children,\n\tthen took his own life. His bride was never found.\n\tThe Mansion has never found peace.", false);
		else if (first && room != LIBRARY && room != GRAND_CORRIDOR) {
			player.health = minOf(100.f, player.health + 40.f);
			player.fear = maxOf(0.f, player.fear - 15.f);
			audio.play(SFX_PICKUP, 85);
			showToast("+ Health kit  (+40 health)");
			say("Drawer:\n\tA small tin first-aid kit, dated 1953.\n\tSomeone hid it here, hoping to live.");
		}
		else if (std::rand() % 3 == 0) {
			say("A cold hand closes around yours from inside the drawer!");
			player.fear += 5; audio.play(SFX_STING, 80); shake = 0.6f;
		}
		else say("The drawer is empty. Something scratched 'SLEEP' inside it.");
		break;
	}
	case 1: // chest
		if (room == COLLAPSED_BEDROOM) { pickUp(BRASS_KEY, "Chest:\n\tA brass key. It opens the nursery.", false); changeFacingTile(sf::Vector2i(2, 1)); }
		else if (room == NURSERY) { pickUp(CHAPEL_KEY, "Chest:\n\tThe Chapel Key, beneath a doll with no face.", false); changeFacingTile(sf::Vector2i(2, 1)); }
		else say("The chest is locked tight.");
		break;
	case 2: say("Empty. Only the smell of ash."); break;
	case 3: // papers
		if (room == CHAPEL) {
			pickUp(HYMN_PAGE, "", false);
			// DFS over the house INCLUDING hidden passages: the hymn lets you
			// 'sense' rooms you cannot see yet.
			lastDfsLen = house.dfsExplore(CHAPEL, lastDfs, true);
			bool found = false;
			for (int i = 0; i < lastDfsLen; ++i) if (lastDfs[i] == HIDDEN_TUNNEL) found = true;
			if (found && house.edgeTo(CHAPEL, HIDDEN_TUNNEL)->hidden) {
				house.reveal(CHAPEL, HIDDEN_TUNNEL);
				audio.play(SFX_SLAM, 90); shake = 1.f;
				say("Hymn Page:\n\t'Beneath the altar the dead still walk.'\n\nAs you read it aloud the house shudders...\nthe narrow door in the chapel's south wall swings open.\n(DFS sensed a hidden passage: the Hidden Tunnel)");
			}
			else say("Hymn Page:\n\t'Beneath the altar the dead still walk.'");
		}
		else say("Police, priest, investigator - all tried to end this.\nSome lost their lives doing so.\nSome say a Devil lives here.");
		break;
	case 4: case 5: case 6: case 7: case 8: case 9: tryDoor(id - 3); break;
	case 10: // book
		if (room == LIBRARY && !collected(DIARY)) pickUp(DIARY, "Diary:\n\tThere are dead souls wandering in the Mansion.\n\tSome force stops them from leaving.\n\tShe still wears her wedding dress...", false);
		else if (room == COLLAPSED_BEDROOM) say("A note: 'The chest opens for those who dare the floor.'");
		else if (room == ANTECHAMBER) say("All attempts to save the house have failed.\nThe Devil is strongest in the next room!\n\nI could take the petrol and burn it all down...\nOr take the Blessed Sword and FIGHT THE DEVIL.");
		else say("The pages are glued together with something dark.");
		break;
	case 11: pickUp(LANTERN, "Lantern picked up.\nIts flame burns a sickly red.\n(F: flare it to drive spirits back)", true); break;
	case 12: // skeleton: Bride's remains (Ossuary) / Devil's corpse (Crypt)
		if (room == OSSUARY && !player.has(CRIMSON_LOCKET)) {
			pickUp(CRIMSON_LOCKET, "The skeleton's fingers open on their own.\nA Crimson Locket. Inside: a wedding photo, 1953.\n\nSomewhere above, a woman stops weeping.", false);
			bride.pacified = true;
			audio.play(SFX_CHIME, 80);
		}
		else if (room == CRIMSON_CRYPT) {
			if (player.has(SWORD)) { player.end_sound.play(); finish(player.has(CRIMSON_LOCKET) ? END_TRUE : END_ESCAPE); }
			else say("The corpse twitches. You need a blessed blade.");
		}
		else say("Bones. Just bones. Probably.");
		break;
	case 15:
		say("A rusted mattock, its handle snapped.\nWhoever dug here never finished. Only the Blessed Sword will do.");
		break;
	case 16: pickUp(SWORD, "The Blessed Sword.\nThe crypt door will open for you now.", true); break;
	case 17:
		audio.play(SFX_LEVER, 90);
		say("Lever pulled! The spikes sink into the floor.");
		for (int r = 0; r < GRID_H; r++) for (int c = 0; c < GRID_W; c++) if (maps[room].map.cell[r][c].x == 21) maps[room].map.cell[r][c] = sf::Vector2i(21, 0);
		changeFacingTile(sf::Vector2i(17, 2));
		break;
	case 18: if (room == CHAPEL) pickUp(IRON_KEY, "The Iron Key. It opens the Ritual Antechamber.", true); break;
	case 19:
		audio.play(SFX_PAPER, 80);
		if (room == ANTECHAMBER) say("Choose wisely - your choice decides how this ends.");
		else if (room == CRIMSON_CRYPT) say("Scrawled in blood:\n'He cannot be dug out or burned. Only the Blessed Sword\nwill end him. Strike true - six times.'  (SHIFT)");
		else if (room == OSSUARY) say("Carved in the stone:\n'Here lies the bride of June 26, 1953.\nReturn her heart to the light and she will rest.'");
		else say("A faint whisper: 'Don't look behind you.'");
		if (room != OSSUARY) changeFacingTile(sf::Vector2i(-1, -1));
		break;
	case 20:
		audio.play(SFX_PICKUP, 80);
		say("You pour the petrol and strike a match...");
		finish(END_TRAPPED);
		break;
	case 23: // wardrobe: hide inside (E again to step out)
		hidden = !hidden;
		player.hiddenAway = hidden;
		if (getenv("CS_DEMO_LOG")) std::fprintf(stderr, "HIDE %d room %d goal %d\n", (int)hidden, room, demoGoal);
		audio.play(SFX_DOOR_OPEN, 55, 1.3f);
		if (hidden) showToast("You hide in the wardrobe. Hold your breath...  (E to step out)");
		break;
	default: say("I don't need this."); break;
	}
}

bool Game::edgeUsable(const Edge& e) {
	if (e.hidden) return false;
	if (!e.locked) return true;
	if (e.requiredItem == NEWSPAPER) return discovered(NEWSPAPER);
	return player.has(e.requiredItem);
}

void Game::tryDoor(int doorNo) {
	Edge* e = house.edgeByDoor(room, doorNo);
	if (!e || e->hidden) {
		say(room == CRIMSON_CRYPT ? "You can't leave. Not now." : "It won't budge. Something holds it from the other side.");
		return;
	}
	if (e->to == OUTSIDE) {
		if (player.has(CRIMSON_LOCKET)) { audio.play(SFX_DOOR_OPEN, 80); finish(END_SECRET); }
		else say("The front door won't open.\nThe house is not done with you.");
		return;
	}
	if (e->locked) {
		if (!edgeUsable(*e)) {
			audio.play(SFX_DOOR_LOCKED, 85);
			rattleT = 0.4f; rattleTile = player.col.facingTile(player.source.y);   // the door rattles in its frame
			if (room == ENTRANCE_HALL) say("It's about to be dark.\nTake the lantern before going any further.");
			else if (room == ANTECHAMBER) say("Make your decision before proceeding.");
			else if (e->requiredItem == NEWSPAPER) say("The door is engraved with a date, half scratched out: 'June 2_, 19_3'.\nSomewhere in this house is a record of that day.");
			else say(Str("Locked. It needs the ") + ITEMS[e->requiredItem].name + ".");
			return;
		}
		int req = e->requiredItem;
		house.unlock(room, e->to);
		audio.play(SFX_UNLOCK, 90);
		bool isKey = req == RUSTY_KEY || req == BRASS_KEY || req == CHAPEL_KEY || req == IRON_KEY;
		if (isKey) { player.take(req); showToast(Str("- ") + ITEMS[req].name + " (used)"); }
	}
	beginDoor(e->to, true, false);
}

void Game::goBack() {
	// Stack: top is the current room; the one beneath it is where we came from.
	int current, previous;
	if (!player.history.pop(&current)) return;
	if (!player.history.top(&previous)) { player.history.push(current); say("There is nowhere to go back to."); return; }
	Edge* e = house.edgeTo(room, previous);
	if (!e || !edgeUsable(*e)) { player.history.push(current); say("The way back is sealed."); return; }
	if (room == CRIMSON_CRYPT) { player.history.push(current); say("You can't leave. Not now."); return; }
	beginDoor(previous, false, true);
}

void Game::flareLantern() {
	if (!player.has(LANTERN)) { say("You have nothing to light."); return; }
	if (flareCooldown > 0) {
		char b[64]; std::snprintf(b, sizeof b, "Flare ready in %.0fs", std::ceil(flareCooldown));
		showToast(b); return;
	}
	flareCooldown = FLARE_COOLDOWN; flare = 1.f;
	lanternOn = true;                    // a flare relights the lantern
	player.fear -= 20;
	audio.play(SFX_FLARE, 90);
	sf::Vector2f c = pp() + sf::Vector2f(12, 12);
	if (bride.visible() && bride.distanceTo(pp()) < 130) {
		bride.banish(frand(25, 35)); runStats.banished++;
		showToast("Samara shrieks and dissolves!");
		audio.play(SFX_SCREAM, 70, 1.3f);
	}
	if (child.active && child.distanceTo(pp()) < 130) {
		// banished: it vanishes completely and only returns much later
		child.active = false; childBanishT = 45.f; runStats.banished++;
		showToast("The flare banishes it - it is gone, for now.");
	}
	for (int i = 0; i < easyCount; ++i) if (dist(ghostEasy[i].characterImage.getPosition(), pp()) < 110) ghostStun = 3.5f;
	for (int i = 0; i < mediumCount; ++i) if (dist(ghostMedium[i].characterImage.getPosition(), pp()) < 110) ghostStun = 3.5f;
	if (bossActive && dist(boss.characterImage.getPosition(), pp()) < 110) { bossStun = 2.f; showToast("The Devil recoils from the light!"); }
	bool stunned = false;
	for (int i = 0; i < spiderCount; ++i) {
		if (!spiders[i].active) continue;
		if (dist(spiders[i].pos, c) < 190) { spiders[i].flareHit(); stunned = true; }   // caught in the light: curls up
		else spiders[i].hear(c, 420.f);                                                 // farther off: it heard that
	}
	if (stunned) showToast("The spider recoils from the light and scuttles back to its nest.");
	(void)c;
}

void Game::finish(int ending) {
	ending_type = ending;
	game_state = ST_ENDING;
	boss.beep_sound.stop();
}

// ---------------------------------------------------------------- hazards
void Game::hurt(float h, float f, const Str& why, int tip, bool respawn) {
	if (player.invulnerable > 0) return;
	player.health -= h; player.fear += f;
	audio.play(SFX_HURT, 85); audio.play(SFX_GASP, 60);
	player.invulnerable = 2.f; hurtFlash = 1.f; shake = 0.8f;
	showToast(why);
	openTip(tip);
	if (player.health <= 0) { die(why, tip); return; }
	if (respawn) placePlayer(entryTile);
}

void Game::die(const Str& why, int tip) {
	if (demo) return;   // the demo dreamer can't die
	player.dead_sound.play();
	boss.beep_sound.stop();
	runStats.deaths++;
	deathReason = why; deathTip = tip;
	game_state = ST_RETRY;
}

void Game::updateHazards() {
	// Forgiving hazards: only the middle of your feet counts (brushing a pit
	// or brazier with the edge of your feet is safe), and you are only hurt if
	// you keep pushing into it for a moment - the first touch is a warning.
	if (!player.isMoving() || player.sliding) { hazardPushT = 0; return; }
	sf::Vector2f c = pp() + sf::Vector2f(12.f, 12.f);          // centre of the 24x24 foot box
	const float reach = 14.f;                                   // just past the edge of the box
	sf::Vector2f ahead = player.source.y == 0 ? sf::Vector2f(0, reach) : player.source.y == 1 ? sf::Vector2f(-reach, 0) :
		player.source.y == 2 ? sf::Vector2f(reach, 0) : sf::Vector2f(0, -reach);
	sf::Vector2f probe = c + ahead;
	sf::Vector2i t = current_map->at((int)(probe.x / 32), (int)(probe.y / 32));
	bool pit = t.y == 1 && t.x == 14;
	bool fire = t.y == 1 && (t.x == 21 || t.x == 22);
	if (!pit && !fire) { hazardPushT = 0; return; }
	hazardPushT += lastDt;
	if (hazardPushT < 0.5f) {
		if (toastT <= 0) showToast(pit ? "You're at the edge of a pit!" : "Too close - it burns!");
		return;
	}
	hazardPushT = 0;
	if (player.invulnerable > 0) return;
	hurt(10, 3, pit ? "You slip into the pit's edge!" : "The flames catch you!", TIP_HAZARDS, false);
	// knocked back a step
	player.characterImage.move(-ahead * 0.8f);
}

void Game::updateDig(float dt) {
	return;   // the crypt is now a sword fight with the Devil - no digging
	if (room != CRIMSON_CRYPT) return;
	int id = player.col.isInteractionMap(pp(), player.source.y);
	if (id != 13) { digT = 0; return; }
	if (!player.is_mattock) { if (dialogue.empty()) say("Find something to dig with - and stand still!"); return; }
	if (player.isMoving()) { digT = 0; return; }
	digT += dt;
	if (digT > 3.5f) {
		digT = 0; digCount++;
		audio.play(SFX_CRACK, 90, 0.6f); shake = 0.5f;
		changeFacingTile(sf::Vector2i(16, 0));
		if (digCount == 2) { player.is_mattock = false; player.take(MATTOCK); say("The mattock broke! Find another tool to dig."); }
		if (digCount >= 4) { changeFacingTile(sf::Vector2i(12, 1)); say("The Devil's corpse lies exposed.\nDestroy it with the Blessed Sword!"); }
	}
}

// ------------------------------------------------------- events & threats
void Game::updateEvents(float dt) {
	(void)dt;
	HauntEvent ev;
	// FIFO: only the oldest pending event may fire.
	while (events.front(&ev) && ev.at <= roomTime) {
		events.dequeue();
		float danger = ROOMS[room].danger / 10.f;
		switch (ev.kind) {
		case EV_FLICKER: player.light.flicker = 1.f; eventCaption = "The lantern gutters..."; player.fear += 2; break;
		case EV_WHISPER: audio.play(SFX_WHISPER, 85); eventCaption = "...stay... sleep with us..."; player.fear += 3; break;
		case EV_SLAM: audio.play(SFX_SLAM, 90); shake = 1.f; eventCaption = "A door slams somewhere above."; player.fear += 3; break;
		case EV_BREATH: audio.play(SFX_BREATH, 60); eventCaption = "Someone whispers your name, right behind you."; player.fear += 3; break;
		case EV_BLOODWRITE: {
			const char* words[] = { "GET OUT", "SHE IS STILL HERE", "1953", "SLEEP", "BEHIND YOU" };
			Str word = words[std::rand() % 5];
			// if the phrase is too long for any wall nearby, scrawl something shorter
			const char* shorter[] = { "GET OUT", "SLEEP", "1953" };
			bool placed = placeWallWriting(word);
			for (int k = 0; !placed && k < 3; ++k) { word = shorter[k]; placed = placeWallWriting(word); }
			if (placed) {
				wallText = word; wallT = 7.f;
				eventCaption = "Something is written on the wall - in blood.";
			}
			else eventCaption = "You feel something being written on the walls...";
			audio.play(SFX_STING, 60); player.fear += 4;
			break;
		}
		case EV_CRACK: audio.play(SFX_CRACK, 80, 0.6f); shake = 0.4f; eventCaption = "The floorboards crack right behind you."; player.fear += 3; break;
		case EV_GLASS: audio.play(SFX_GLASS, 80); shake = 0.3f; eventCaption = "Somewhere in the house, glass shatters."; player.fear += 3; break;
		default: audio.play(SFX_MOAN, 70); eventCaption = "A low moan rises from beneath the floorboards."; player.fear += 2; break;
		}
		player.fear += 1 * danger;
		eventCaptionT = 4.f;
	}
}

void Game::updateThreats(float dt) {
	threats.clear();
	sf::Vector2f p = pp();
	// Insert every active danger with its priority...
	if (bride.active && bride.visible() && !bride.pacified) {
		float d = bride.distanceTo(p);
		if (bride.lostTrail()) { if (d < 240) threats.insert(Threat{ 45, TH_BRIDE_NEAR, "Samara has lost your trail - keep still" }); }
		else if (d < 22) threats.insert(Threat{ 100, TH_BRIDE_ATTACK, "SAMARA HAS YOU" });
		else if (d < 240) threats.insert(Threat{ 75, TH_BRIDE_NEAR, "Samara drifts toward you - stand still, or FLARE (F)" });
	}
	if (bossActive && bossStun <= 0 && dist(boss.characterImage.getPosition(), p) < 150) threats.insert(Threat{ 90, TH_DEVIL, "The Devil is close - the EMF meter screams" });
	if (child.active && child.stun <= 0 && child.distanceTo(p) < 170) {
		threats.insert(Threat{ child.frozen ? 50 : 80, TH_CHILD, child.frozen ? "Carrie is frozen while you watch her" : "Carrie stalks behind you in blood - TURN AND FACE HER" });
		openTip(TIP_CHILD);
	}
	for (int i = 0; i < easyCount; ++i) if (dist(ghostEasy[i].characterImage.getPosition(), p) < 90) threats.insert(Threat{ 60, TH_GHOST, "A restless spirit walks its old path" });
	for (int i = 0; i < mediumCount; ++i) if (dist(ghostMedium[i].characterImage.getPosition(), p) < 90) threats.insert(Threat{ 60, TH_GHOST, "A restless spirit walks its old path" });
	// Spiders: how dangerous depends on what their AI is doing.
	for (int i = 0; i < spiderCount; ++i) {
		const Spider& s = spiders[i];
		if (!s.active) continue;
		if ((s.state == Spider::CHASE || s.state == Spider::ATTACK) && dist(s.pos, p + sf::Vector2f(12, 12)) < 60) { threats.insert(Threat{ 70, TH_SPIDER, "It's at your feet - STOMP (Q)!" }); break; }
		if (s.state == Spider::CHASE || s.state == Spider::ATTACK) { threats.insert(Threat{ 65, TH_SPIDER, "A spider has SEEN you - let it come, then STOMP (Q)" }); break; }
		if (s.state == Spider::INVESTIGATE) { threats.insert(Threat{ 45, TH_SPIDER, "Something heard your footsteps... stand still, lantern off (L)" }); break; }
		if (s.dangerous() && dist(s.pos, p + sf::Vector2f(12, 12)) < 160) { threats.insert(Threat{ 40, TH_SPIDER, "Something with too many legs is close" }); break; }
	}
	if (ROOMS[room].dark && !player.has(LANTERN)) threats.insert(Threat{ 20, TH_DARK, "It is too dark here" });
	threats.insert(Threat{ 0, TH_CALM, "" });

	// ...and react to the single most dangerous one (max-heap peek).
	threats.peek(&topThreat);
	// Chase music: something is actively hunting you.
	bool hunted = topThreat.kind == TH_BRIDE_NEAR || topThreat.kind == TH_BRIDE_ATTACK || topThreat.kind == TH_DEVIL ||
		(topThreat.kind == TH_CHILD && !child.frozen);
	if (hunted) chaseHold = 4.f;
	else if (chaseHold > 0) chaseHold -= dt;
	float rate;
	switch (topThreat.kind) {
	case TH_BRIDE_ATTACK: rate = 0; break;
	case TH_DEVIL: rate = 4; break;
	case TH_CHILD: rate = child.frozen ? 1 : 3.5f; break;
	case TH_BRIDE_NEAR: rate = 2.5f; break;
	case TH_GHOST: rate = 1.5f; break;
	case TH_SPIDER: rate = 2.f; break;
	case TH_DARK: rate = 1; break;
	default: rate = (ROOMS[room].danger <= 3 ? -4.f : -2.f); break;   // calm down when nothing is near
	}
	player.fear += rate * dt;

	if (topThreat.kind == TH_BRIDE_NEAR) openTip(TIP_BRIDE);
	if (topThreat.kind == TH_GHOST) openTip(TIP_SPIRITS);
	if (topThreat.kind == TH_SPIDER) openTip(TIP_SPIDERS);
	if (topThreat.kind == TH_DEVIL) openTip(TIP_DEVIL);
	if (topThreat.kind == TH_BRIDE_ATTACK && player.invulnerable <= 0) {
		jumpscare = 1.3f;
		audio.play(SFX_JUMPSCARE, 100);
		bride.banish(frand(25, 35));
		hurt(30, 18, "Samara's cursed face fills your vision.", TIP_BRIDE, false);
	}
	// Contact with patrol spirits / crawler.
	// A ghost's touch hurts and shoves you back a step - only dying sends you to the room's start.
	for (int i = 0; i < easyCount && ghostStun <= 0 && !hidden; ++i) if (ghostEasy[i].isCollisionCharacter(&player) && player.invulnerable <= 0) {
		hurt(25, 10, "A spirit passes through you. Ice in your veins.", TIP_SPIRITS, false);
		knockBack(ghostEasy[i].characterImage.getPosition());
	}
	for (int i = 0; i < mediumCount && ghostStun <= 0 && !hidden; ++i) if (ghostMedium[i].isCollisionCharacter(&player) && player.invulnerable <= 0) {
		hurt(25, 10, "A spirit passes through you. Ice in your veins.", TIP_SPIRITS, false);
		knockBack(ghostMedium[i].characterImage.getPosition());
	}
	if (child.active && child.stun <= 0 && !hidden && child.distanceTo(p) < 18) {
		audio.play(SFX_CHILD_LUNGE, 95);
		child.stun = 3; child.pos = child.home;
		hurt(25, 10, "Carrie's telekinetic wrath violently slams you!", TIP_CHILD, false);
	}

	if (player.fear < 0) player.fear = 0;
	if (player.fear > runStats.peakFear) runStats.peakFear = player.fear;
	if (player.fear > 55 && !tipSeen[TIP_FEAR]) openTip(TIP_FEAR);
	if (player.fear >= 100) { player.fear = 100; die("Your mind shattered. You laugh, and cannot stop.", TIP_FEAR); return; }

	// Heartbeat speeds up with fear.
	audio.setTension(player.fear / 100.f);
	heartbeatT -= dt;
	if (player.fear > 50 && heartbeatT <= 0) {
		audio.play(player.fear > 75 ? SFX_HEART_FAST : SFX_HEART_SLOW, 40 + player.fear * 0.5f);
		heartbeatT = 1.6f - player.fear / 100.f;
	}
}

// The first time you meet a horror its survival note slides in at the side -
// the game keeps going.  (force: open the full page, e.g. from the notes.)
void Game::openTip(int tip, bool force) {
	if (!force && tipSeen[tip]) return;
	tipSeen[tip] = true;
	if (force) { tipOpen = tip; return; }
	tipCard = tip; tipCardT = TIP_CARD_TIME;
	audio.play(SFX_PAPER, 60);
}

// ------------------------------------------------------------------ hints
struct Step { int item; int room; int tile; int lockFrom; int lockTo; };
static const Step STEPS[] = {
	{ LANTERN,    ENTRANCE_HALL,     11, ENTRANCE_HALL,  GRAND_CORRIDOR },
	{ RUSTY_KEY,  LIBRARY,            0, LIBRARY,        COLLAPSED_BEDROOM },
	{ BRASS_KEY,  COLLAPSED_BEDROOM,  1, LIBRARY,        NURSERY },
	{ CHAPEL_KEY, NURSERY,            1, GRAND_CORRIDOR, CHAPEL },
	{ IRON_KEY,   CHAPEL,            18, GRAND_CORRIDOR, ANTECHAMBER },
	{ SWORD,      ANTECHAMBER,       16, ANTECHAMBER,    CRIMSON_CRYPT },
};

void Game::updateHints(float dt) {
	// Work out the current objective: first key not yet found, or found
	// but its door still locked.
	int key = 100; targetRoom = room; targetTile = -1;
	Str what;
	for (int i = 0; i < 6; ++i) {
		const Step& s = STEPS[i];
		Edge* e = house.edgeTo(s.lockFrom, s.lockTo);
		if (!collected(s.item)) { key = i * 2; targetRoom = s.room; targetTile = s.tile; what = Str("The ") + ITEMS[s.item].name + " is somewhere in the " + ROOMS[s.room].name + "."; break; }
		if (e && e->locked) { key = i * 2 + 1; targetRoom = s.lockFrom; targetTile = 3 + e->doorNo; what = Str("Use the ") + ITEMS[s.item].name + " on the door to the " + ROOMS[s.lockTo].name + "."; break; }
	}
	if (key == 100) {
		targetRoom = CRIMSON_CRYPT;
		int lc, lr;
		if (maps[CRIMSON_CRYPT].map.find(19, &lc, &lr)) { key = 20; targetTile = 19; what = "Read the note at the crypt's entrance - it opens the way."; }
		else if (maps[CRIMSON_CRYPT].map.find(17, &lc, &lr)) { key = 21; targetTile = 17; what = "Pull the crypt lever to lower the spikes."; }
		else { key = 22 + (BOSS_MAX_HP - bossHP); targetTile = -1; what = "Kill the Devil with the Blessed Sword: SHIFT to strike. A flare (F) stuns him."; }
	}
	// In the Nursery the chest is ringed by spikes until the lever is pulled.
	if (targetRoom == NURSERY && targetTile == 1) {
		int lc, lr;
		if (maps[NURSERY].map.find(17, &lc, &lr)) { targetTile = 17; what = "Spikes guard the chest in the Nursery. Pull the lever on the right to lower them first."; }
	}
	if (key != objectiveKey) { objectiveKey = key; objectiveTime = forceHint ? 125.f : 0.f; forceHint = false; hintText = ""; trailLen = 0; }
	objectiveTime += dt;
	objectiveText = what;

	// After 30 seconds without progress, the house "remembers" the way for you.
	if (objectiveTime >= 30) {
		Str route;
		if (targetRoom != room) {
			lastRouteLen = house.bfsPath(room, targetRoom, lastRoute, MAX_ROOMS, [this](const Edge& e) { return edgeUsable(e); });
			if (lastRouteLen > 1) {
				route = "  Route: ";
				for (int i = 0; i < lastRouteLen; ++i) { route += ROOMS[lastRoute[i]].name; if (i + 1 < lastRouteLen) route += " > "; }
			}
		}
		else { route = "  Follow the trail of blood."; lastRoute[0] = room; lastRouteLen = 1; }   // already in the right room
		hintText = "HINT: " + what + route;
		// The trail is worked out once and then stays put; it is only redone
		// when you leave the room or wander well away from it.
		if (room != targetRoom) trailLen = 0;
		else if (trailLen == 0 || trailDistance() > 2) computeTrail();
	}
}

// Tile-level BFS from the player to the objective tile (Queue + parents).
void Game::computeTrail() {
	int tc, tr;
	trailLen = 0;
	if (targetTile < 0 || !current_map->find(targetTile, &tc, &tr)) return;
	int sc = (int)((pp().x + 12) / 32), sr = (int)((pp().y + 12) / 32);
	static int parent[GRID_W * GRID_H];
	for (int i = 0; i < GRID_W * GRID_H; ++i) parent[i] = -2;
	Queue<int> q;
	q.enqueue(sr * GRID_W + sc); parent[sr * GRID_W + sc] = -1;
	int goal = -1, cell;
	const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
	while (q.dequeue(&cell)) {
		int c = cell % GRID_W, r = cell / GRID_W;
		if ((std::abs(c - tc) + std::abs(r - tr)) == 1) { goal = cell; break; }
		for (int k = 0; k < 4; ++k) {
			int nc = c + dx[k], nr = r + dy[k];
			if (!current_map->inside(nc, nr) || current_map->solid(nc, nr)) continue;
			int ni = nr * GRID_W + nc;
			if (parent[ni] != -2) continue;
			parent[ni] = cell;
			q.enqueue(ni);
		}
	}
	for (int c = goal; c >= 0 && trailLen < GRID_W * GRID_H; c = parent[c]) trail[trailLen++] = c;
}

// ------------------------------------------------------------- Alfred
// Returns true if this frame's E press was used to start talking to him.
bool Game::updateAlfred(float dt, sf::RenderWindow* window, bool overlay) {
	bool talked = false;
	if (!overlay) {
		if (room == ENTRANCE_HALL && npc.state == Npc::standing && npc.near(&player) &&
			(key_hit(sf::Keyboard::Key::E) || key_hit(sf::Keyboard::Key::Enter))) {
			npc.state = Npc::counting;
			alfredT = 0;
			alfredWalkT = 0;
			player.lockInput = true;      // you walk over to him and stay for the talk
			talked = true;
			say("Alfred:\n\tYou came... thank God.\n\tThe house has slept in crimson since 1953.\n\tYou are our only hope.");
		}
		if (alfredT >= 0) {
			// Walk the player over to Alfred and keep them beside him.
			sf::Vector2f d = npc.characterImage.getPosition() - pp();
			bool horiz = std::fabs(d.x) > std::fabs(d.y);
			int toward = horiz ? (d.x > 0 ? Character::Right : Character::Left) : (d.y > 0 ? Character::Down : Character::Up);
			alfredWalkT += dt;
			if (std::sqrt(d.x * d.x + d.y * d.y) > 28.f && alfredWalkT < 3.f) player.autoDir = toward;
			else { player.autoDir = -1; player.source.y = toward; }   // stand still, facing him
			// Space jumps to his next line, X skips the whole talk.
			if (!talked && key_hit(sf::Keyboard::Key::Space)) {
				const float marks[] = { 5.f, 11.f, 18.f };
				for (float m : marks) if (alfredT < m) { alfredT = m - 0.0001f; break; }
			}
			if (!talked && key_hit(sf::Keyboard::Key::X)) {
				alfredGiveLantern();
				alfredT = 18.f - 0.0001f;
			}
			float before = alfredT;
			alfredT += dt;
			auto crossed = [&](float t) { return before < t && alfredT >= t; };
			if (crossed(5.f)) {
				alfredGiveLantern();   // he hands you the lantern himself
				say("Alfred:\n\tTake my lantern. The lines were cut.\n\tPress F to flare it - the dead hate the light.");
			}
			if (crossed(11.f))
				say("Alfred:\n\tE to open doors, drawers and chests.\n\tB takes you back the way you came.\n\tIf you are lost, press H - the house remembers the way.");
			if (crossed(18.f)) {
				say("Alfred:\n\tIt's getting dark. I... I should go.");
				npc.state = Npc::moving;
				alfredT = -1;
				player.lockInput = false;     // you're free to move again
				player.autoDir = -1;
			}
		}
		// Once he has walked out of the hall, the house takes him.
		if (!alfredLeft && npc.state == Npc::moving && npc.characterImage.getPosition().y >= 644) {
			alfredLeft = true;
			alfredScreamT = 25.f;
		}
		if (alfredScreamT > 0) {
			alfredScreamT -= dt;
			if (alfredScreamT <= 0) {
				audio.play(SFX_SCREAM, 55, 0.75f);
				eventCaption = "A man screams somewhere in the house... Alfred?";
				eventCaptionT = 5.f;
				player.fear += 5;
				alfredDead = 1;
			}
		}
	}
	if (room == ENTRANCE_HALL && npc.characterImage.getPosition().y < 644) {
		if (overlay) window->draw(npc.characterImage); else npc.movement(&player, window);
	}
	return talked;
}

// Dead Alfred waits in the Library, back turned, in the dark.  Get close
// and he turns around.
void Game::drawDeadAlfred(sf::RenderWindow* window, float dt, bool overlay) {
	if (room != LIBRARY || alfredDead == 0 || alfredDead == 3) return;
	const sf::Vector2f pos(9 * 32 + 4.f, 11 * 32 + 4.f);
	alfredDeadSprite.setPosition(pos);
	if (alfredDead == 1) {
		alfredDeadSprite.setTextureRect(sf::IntRect({ 0, 3 * 48 }, { 24, 48 }));   // back turned
		alfredDeadSprite.setColor(sf::Color::White);
		sf::Vector2f d = pp() - pos;
		if (!overlay && std::sqrt(d.x * d.x + d.y * d.y) < 60.f) {
			alfredDead = 2; alfredDeadT = 0;
			audio.play(SFX_STING, 100, 0.7f);
			shake = 1.f; hurtFlash = 0.5f;
			player.fear += 8;
			eventCaption = "Alfred turns around. His eyes are gone.";
			eventCaptionT = 5.f;
		}
	}
	else if (alfredDead == 2) {
		if (!overlay) alfredDeadT += dt;
		int frame = (int)(alfredDeadT * 8) % 3;
		alfredDeadSprite.setTextureRect(sf::IntRect({ frame * 24, 0 }, { 24, 48 }));   // facing you
		float a = alfredDeadT < 1.4f ? 1.f : maxOf(0.f, 1.f - (alfredDeadT - 1.4f) / 0.6f);
		bool flick = alfredDeadT > 1.0f && std::rand() % 3 == 0;
		alfredDeadSprite.setColor(sf::Color(255, 255, 255, (std::uint8_t)(flick ? 60 : 255 * a)));
		if (alfredDeadT > 2.0f) alfredDead = 3;
	}
	window->draw(alfredDeadSprite);
}

// --------------------------------------------------- footsteps & voices
void Game::updateVoices(float dt) {
	// footsteps on this room's floor
	if (player.isMoving()) {
		stepT -= dt;
		if (stepT <= 0) { audio.play(ROOM_STEP[room], 30); stepT = 0.36f; }
	}
	else stepT = 0;
	// the Bride announces herself, then whispers as she hunts
	if (bride.active) {
		if (bride.state == Bride::FADE_IN && lastBrideState == Bride::GONE) audio.play(SFX_BRIDE_APPEAR, 70);
		if (bride.state == Bride::HUNT && !bride.pacified) {
			brideVoiceT -= dt;
			if (brideVoiceT <= 0) {
				float d = bride.distanceTo(pp());
				audio.play(SFX_BRIDE, maxOf(20.f, 85.f - d * 0.25f), 1.45f);   // pitched up: a woman weeping
				brideVoiceT = frand(5, 9);
			}
		}
		lastBrideState = bride.state;
	}
	// the crawler mutters when it moves
	if (child.active && !child.frozen && child.distanceTo(pp()) < 220) {
		childVoiceT -= dt;
		if (childVoiceT <= 0) { audio.play(SFX_CHILD, 65); childVoiceT = frand(3, 6); }
	}
	// the Devil moans when he is near
	if (bossActive && dist(boss.characterImage.getPosition(), pp()) < 260) {
		devilVoiceT -= dt;
		if (devilVoiceT <= 0) { audio.play(SFX_MOAN, 75, 0.8f); devilVoiceT = frand(6, 10); }
	}
}

// How far (in tiles) the player is from the nearest tile of the current trail.
int Game::trailDistance() const {
	int pc = (int)((pp().x + 12) / 32), pr = (int)((pp().y + 12) / 32), best = 999;
	for (int i = 0; i < trailLen; ++i) {
		int d = std::abs(trail[i] % GRID_W - pc) + std::abs(trail[i] / GRID_W - pr);
		if (d < best) best = d;
	}
	return best;
}

void Game::alfredGiveLantern() {
	if (collected(LANTERN)) return;
	pickUp(LANTERN, "", false);
	int c, r;
	if (maps[ENTRANCE_HALL].map.find(11, &c, &r)) maps[ENTRANCE_HALL].map.set(c, r, sf::Vector2i(-1, -1));
}

// Finds a stretch of wall face (tile 3,3) near the player that is long enough
// for the words, and centres the writing on it.  False if there is none.
bool Game::placeWallWriting(const Str& word) {
	float need = scrawlWidth(word) + 10.f;
	sf::Vector2f me = pp() + sf::Vector2f(12.f, 12.f);
	float best = 1e9f; bool found = false;
	for (int r = 0; r < GRID_H; ++r) {
		int c = 0;
		while (c < GRID_W) {
			if (current_map->at(c, r) != sf::Vector2i(3, 3)) { ++c; continue; }
			int start = c;
			while (c < GRID_W && current_map->at(c, r) == sf::Vector2i(3, 3)) ++c;
			float len = (c - start) * 32.f;
			if (len < need) continue;
			// slide the text along the run to the spot closest to the player
			float x0 = start * 32.f, x1 = c * 32.f - need;
			float x = maxOf(x0, minOf(x1, me.x - need / 2));
			float y = r * 32.f;
			float dx = x + need / 2 - me.x, dy = y + 16 - me.y;
			float d = std::sqrt(dx * dx + dy * dy) + frand(0, 40);   // a little variety
			if (d < 230 && d < best) { best = d; found = true; wallPos = { x + 4.f, y + 4.f }; }   // close enough to be on screen
		}
	}
	return found;
}

// Width of a word written in the blood font, letter by letter (as it is drawn).
float Game::scrawlWidth(const Str& word) const {
	float x = 0;
	for (char ch : word) x += fontBlood.getGlyph((unsigned char)ch, WALL_TEXT_SIZE, false).advance + (ch == ' ' ? 7.f : 1.f);
	return x;
}

// Pushes the player up to 28px directly away from `from`, one pixel at a time,
// stopping at anything solid (walls, pits, fire), so a hit never throws you
// into danger or back to the room's entrance.
void Game::knockBack(sf::Vector2f from) {
	if (demo) return;
	sf::Vector2f d = pp() - from;
	bool horiz = std::fabs(d.x) >= std::fabs(d.y);
	int dir = horiz ? (d.x >= 0 ? Character::Right : Character::Left) : (d.y >= 0 ? Character::Down : Character::Up);
	sf::Vector2f step = dir == Character::Right ? sf::Vector2f(1, 0) : dir == Character::Left ? sf::Vector2f(-1, 0) :
		dir == Character::Down ? sf::Vector2f(0, 1) : sf::Vector2f(0, -1);
	for (int i = 0; i < 28; ++i) {
		if (player.col.isCollisionMap(pp(), dir)) break;
		player.characterImage.move(step);
	}
}

// A stunned spider, stepped on.  It stays where it died - for good.
void Game::squash(Spider& s) {
	s.active = false;
	if (corpseCount[room] < 4) corpses[room][corpseCount[room]++] = s.pos;
	audio.play(SFX_CRACK, 90, 1.6f);
	audio.play(SFX_GASP, 50, 1.4f);
	shake = 0.4f;
	runStats.banished++;
	showToast("SQUASHED.  It won't crawl again.");
}

// Q: stamp on the nearest spider if it's right at your feet.  Miss, and it bites.
void Game::stomp() {
	if (hidden) return;
	if (stompT > 0) {                              // still recovering from the last stomp
		char b[48]; std::snprintf(b, sizeof b, "Stomp ready in %.0fs", std::ceil(stompT));
		showToast(b); return;
	}
	stompT = STOMP_COOLDOWN;                       // hit or miss, you need a moment to stomp again
	player.startStomp();                           // lift... (the hit lands on the STAMP frame)
	stompHitT = Player::STOMP_FRAME;
}
void Game::stompLand() {                           // the boot comes down
	shake = maxOf(shake, 0.3f);
	stompFxT = 0.35f;
	audio.play(SFX_STEP_WOOD, 95, 0.7f);           // a heavy boot
	sf::Vector2f me = pp() + sf::Vector2f(12, 12);
	Spider* best = nullptr; float bestD = STOMP_REACH;   // reach: right at your feet (shown as a ring)
	for (int i = 0; i < spiderCount; ++i)
		if (spiders[i].active && dist(spiders[i].pos, me) < bestD) { bestD = dist(spiders[i].pos, me); best = &spiders[i]; }
	if (best) squash(*best);
	else showToast("You stamp at nothing...");
}

// SHIFT: swing the Blessed Sword.  The blow lands on the STRIKE frame (the 3rd).
void Game::swingSword() {
	if (hidden || !player.has(SWORD) || swingT > 0 || player.swinging) return;
	swingT = 0.45f;
	player.startSwing();
	swingHitT = 2 * Player::SWING_FRAME;
	audio.play(SFX_WHISPER, 55, 1.8f);             // the blade cuts the air
}
void Game::swordLand() {
	sf::Vector2f me = pp() + sf::Vector2f(12, 12);
	int d = player.source.y;
	sf::Vector2f f = d == Character::Down ? sf::Vector2f(0, 1) : d == Character::Left ? sf::Vector2f(-1, 0) :
	                 d == Character::Right ? sf::Vector2f(1, 0) : sf::Vector2f(0, -1);
	auto inReach = [&](sf::Vector2f c, float reach) {   // in front of you, within the blade's sweep
		sf::Vector2f v = c - me;
		float l = std::sqrt(v.x * v.x + v.y * v.y);
		return l < 22 || (l < reach && (v.x * f.x + v.y * f.y) / l > 0.2f);
	};
	bool hit = false;
	if (bossActive && inReach(boss.characterImage.getPosition() + sf::Vector2f(12, 0), 58)) {
		hit = true;
		--bossHP; bossHitT = 0.25f; bossStun = maxOf(bossStun, 0.7f);
		shake = 0.7f;
		audio.play(SFX_SLAM, 95, 0.7f);
		for (int i = 0; i < 26; ++i) {                // knocked back along the blow
			sf::Vector2f step = f * 1.f;
			int dir = d;
			if (boss.col.isCollisionMap(boss.characterImage.getPosition(), dir)) break;
			boss.characterImage.move(step);
		}
		if (bossHP <= 0) {
			bossActive = false; bossDeathT = 0; boss.beep_sound.stop();
			audio.play(SFX_SCREAM, 100, 0.5f);
			say("The Blessed Sword bites deep.\nThe Devil shrieks, and the crypt shakes...");
			runStats.banished++;
		}
		else {
			char b[64]; std::snprintf(b, sizeof b, "The blade bites!  (%d more)", bossHP);
			showToast(b);
		}
	}
	for (int i = 0; i < spiderCount; ++i)          // the sword cuts spiders down too
		if (spiders[i].active && inReach(spiders[i].pos, 50)) { squash(spiders[i]); hit = true; }
	for (int i = 0; i < easyCount; ++i) if (inReach(ghostEasy[i].characterImage.getPosition() + sf::Vector2f(12, 0), 50)) { ghostStun = 2.f; hit = true; }
	for (int i = 0; i < mediumCount; ++i) if (inReach(ghostMedium[i].characterImage.getPosition() + sf::Vector2f(12, 0), 50)) { ghostStun = 2.f; hit = true; }
	if (!hit) audio.play(SFX_STEP_STONE, 50, 1.5f);
}

// ------------------------------------------------------------------ doors
static int findDoorTile(const TileGrid& m, int doorNo, sf::Vector2i* out) {
	int c, r;
	if (m.find(3 + doorNo, &c, &r)) { *out = sf::Vector2i(c, r); return 1; }
	*out = sf::Vector2i(-1, -1); return 0;
}
void Game::beginDoor(int to, bool push, bool back) {
	if (doorT >= 0) return;
	doorTo = to; doorPush = push; doorBack = back; doorT = 0;
	doorTile = player.col.facingTile(player.source.y);
	if (back) {                                    // B: the door you walk back through
		const Edge* e = house.edgeTo(room, to);
		if (e) findDoorTile(maps[room].map, e->doorNo, &doorTile);
	}
	audio.play(SFX_DOOR_OPEN, 80);
	walkFrom = pp();                               // the hero will walk into the doorway
	walkTo = sf::Vector2f(doorTile.x * 32.f + 4, doorTile.y * 32.f + 4);
	if (getenv("CS_DEMO_LOG")) std::fprintf(stderr, "DOOR open at %.2f s\n", runStats.time);
}
static float smooth(float k) { k = k < 0 ? 0 : k > 1 ? 1 : k; return k * k * (3 - 2 * k); }
static int faceToward(sf::Vector2f d) {
	return std::fabs(d.x) > std::fabs(d.y) ? (d.x > 0 ? Character::Right : Character::Left) : (d.y > 0 ? Character::Down : Character::Up);
}
void Game::updateDoor(float dt) {
	if (rattleT > 0) rattleT -= dt;
	if (arriveT >= 0) {
		float before = arriveT;
		arriveT += dt;
		if (arriveTile.x >= 0 && arriveT < WALK_OUT) {          // stepping out of the doorway into the room
			player.scriptedWalk = true;
			player.source.y = faceToward(walkTo - walkFrom);
			player.setPosition(walkFrom + (walkTo - walkFrom) * smooth(arriveT / WALK_OUT));
		}
		else if (player.scriptedWalk) { player.scriptedWalk = false; player.setPosition(walkTo); }
		if (before < WALK_OUT + DOOR_IN && arriveT >= WALK_OUT + DOOR_IN) audio.play(SFX_SLAM, 35, 1.3f);   // a soft thud as it shuts
		if (arriveT > WALK_OUT + DOOR_IN + 0.3f) arriveT = -1;
	}
	if (doorT < 0) return;
	doorT += dt;
	// the door swings open; once it's part-way, the hero walks into the doorway and the dark
	float walk = smooth((doorT - 0.18f) / (DOOR_OPEN + DOOR_FADE - 0.18f));
	player.scriptedWalk = walk > 0 && walk < 1;
	player.source.y = faceToward(walkTo - walkFrom);
	player.setPosition(walkFrom + (walkTo - walkFrom) * walk);
	if (doorT >= DOOR_OPEN + DOOR_FADE) {          // black: change rooms, then walk out of the door there
		int from = room;
		doorT = -1;
		player.scriptedWalk = false;
		enterRoom(doorTo, from, doorPush);
		if (doorBack) showToast(Str("Went back to ") + ROOMS[doorTo].name);
		const Edge* e = house.edgeTo(room, from);
		arriveTile = sf::Vector2i(-1, -1);
		if (e) findDoorTile(maps[room].map, e->doorNo, &arriveTile);
		walkTo = pp();                             // where enterRoom put you, just inside
		walkFrom = arriveTile.x >= 0 ? sf::Vector2f(arriveTile.x * 32.f + 4, arriveTile.y * 32.f + 4) : walkTo;
		if (arriveTile.x >= 0) player.setPosition(walkFrom);
		arriveT = 0;
	}
}
// Animates the game's own painted door: it folds toward its hinge as it swings
// away into the dark doorway (darkening as it turns), and unfolds again as it
// shuts.  Front doors (top / bottom walls) fold toward the left; side-wall doors,
// drawn edge-on, fold upward.  open: 0 shut .. 1 open; jitter rattles it.
void Game::drawDoorTile(sf::RenderWindow* w, sf::Vector2i t, float open, float jitter, bool away) {
	if (t.x < 0) return;
	const TileGrid& m = maps[room].map;
	sf::Vector2i v = m.at(t.x, t.y);
	if (v.y != 1) return;
	auto floor = [&](int c, int r) { sf::Vector2i u = m.at(c, r); return m.inside(c, r) && !m.solid(c, r) && !(u.x == 0 && u.y == 0); };
	bool sideDoor = !floor(t.x, t.y + 1) && !floor(t.x, t.y - 1);    // room is left/right of it
	sf::Vector2f p(t.x * 32.f, t.y * 32.f);
	// the doorway behind the door (side doors: only the narrow door strip)
	sf::RectangleShape gap(sideDoor ? sf::Vector2f(11.f, 32.f) : sf::Vector2f(32.f, 32.f));
	gap.setPosition(sideDoor ? p + sf::Vector2f(11, 0) : p);
	gap.setFillColor(sf::Color(3, 1, 2)); w->draw(gap);
	float ease = open * open * (3 - 2 * open);           // smoothstep
	float k = 1.f - 0.86f * ease;                        // how much of the door still faces you
	std::uint8_t shade = (std::uint8_t)(255 - 120 * ease);   // it darkens as it turns away from the light
	sf::Sprite door(tileset);
	if (sideDoor) {
		// Seen from above, a side-wall door is a narrow strip lying along the wall.
		// It ROTATES on its top hinge out of the wall and into the room (like a
		// clock hand), 90 degrees: toward the right if the room is on the right.
		bool roomRight = floor(t.x + 1, t.y);
		door.setTextureRect(sf::IntRect({ v.x * 32 + 11, 32 }, { 11, 32 }));
		door.setOrigin({ 5.5f, 0.f });
		door.setPosition(p + sf::Vector2f(16.5f + jitter, 1.f));
		// You push a door open: it swings AWAY from you, into the next room.  The door
		// behind you on arrival was pushed from the other side, so it opens toward you.
		float intoRoom = roomRight ? -90.f : 90.f;
		door.setRotation(sf::degrees((away ? -intoRoom : intoRoom) * ease));
		shade = (std::uint8_t)(255 - 50 * ease);     // it stays lit - it's turning toward you, not away
	}
	else {
		door.setTextureRect(sf::IntRect({ v.x * 32, 32 }, { 32, 32 }));
		door.setPosition(p + sf::Vector2f(jitter, 0));
		door.setScale({ k, 1.f });                       // hinge on the left
	}
	door.setColor(sf::Color(shade, shade, shade));
	w->draw(door);
	if (ease > 0.05f && !sideDoor) {                     // the door's edge catching the light as it turns
		sf::RectangleShape edge(sideDoor ? sf::Vector2f(11.f, 1.f) : sf::Vector2f(1.f, 32.f));
		edge.setPosition(sideDoor ? p + sf::Vector2f(11, 32 * k - 1) : p + sf::Vector2f(32 * k - 1, 0));
		edge.setFillColor(sf::Color(170, 120, 90, (std::uint8_t)(200 * ease))); w->draw(edge);
	}
}
