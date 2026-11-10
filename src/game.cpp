#include "game.h"
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
	  "- A touch costs blood and courage - not your life." },
	{ "The Weeping Bride",
	  "She drifts through walls, hunting by the sound of your steps.\n"
	  "- STAND STILL and she loses your trail.\n"
	  "- FLARE your lantern (F) when she is close to banish her.\n"
	  "- Her locket lies somewhere below the chapel. Return it to her\n"
	  "  and she will never harm you again." },
	{ "The Hollow Child",
	  "It crawls faster than you can run - but only when unseen.\n"
	  "- FACE it and it cannot move.\n"
	  "- Walk backwards toward the door, eyes on the child.\n"
	  "- A FLARE (F) drives it back into the dark." },
	{ "The Devil",
	  "He cannot be fought - only ended.\n"
	  "- The EMF meter beeps when he is close: keep your distance.\n"
	  "- Dig at the cracked heart of the crypt with a mattock\n"
	  "  (stand still on it). Mattocks break - there is a spare.\n"
	  "- Strike the corpse with the Blessed Sword. A flare stuns him." },
	{ "Spikes, Fire and Graves",
	  "The house itself wants you.\n"
	  "- Spikes and flames rise and fall - time your steps.\n"
	  "- Some rooms have a lever that silences the spikes." },
	{ "Fear",
	  "Fear rises in the dark, near the dead, and with every scare.\n"
	  "- At 100 fear your mind breaks.\n"
	  "- Keep the lantern lit and rest in quiet rooms to calm down.\n"
	  "- Flaring the lantern steadies your nerves." },
};

// ------------------------------------------------------------------ setup
Game::Game(Audio& a) : audio(a) {
	(void)tileset.loadFromFile(asset("graphics/texture.png"));
	for (int r = 1; r < ROOM_COUNT; ++r) maps[r].initializeMap(ROOMS[r].mapFile, tileset);
	buildHouse(house);
	buildItemDatabase(itemDB);
	(void)fontTitle.openFromFile(asset("fonts/GrenzeGotisch.ttf"));
	(void)fontSerif.openFromFile(asset("fonts/CormorantGaramond.ttf"));
	(void)fontItalic.openFromFile(asset("fonts/CormorantGaramond-Italic.ttf"));
	(void)fontMono.openFromFile(asset("fonts/DejaVuSans.ttf"));
	if (jumpTex.loadFromFile(asset("graphics/jumpscare.png"))) jumpSprite.setTexture(jumpTex, true);
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
	room = to;
	current_map = &maps[room].map;
	entryTile = from < 0 ? sf::Vector2i(ROOMS[room].spawnX, ROOMS[room].spawnY) : spawnNearDoor(room, from);
	placePlayer(entryTile);
	if (pushHistory) player.history.push(room);
	if (!roomVisited[room]) { roomVisited[room] = true; runStats.roomsSeen++; }
	roomTime = 0; digT = 0; trailLen = 0;
	spawnEnemies();

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
	case NURSERY: say("A music box plays by itself.\nI must not come in contact with whatever lives here."); break;
	case CRIMSON_CRYPT: say("The Devil is somewhere around.\nThe EMF meter beeps when he comes close.\nDig out his corpse at the crypt's heart and destroy it."); break;
	case HIDDEN_TUNNEL: say("Wet stone. Scratch marks. Someone was walled in here."); break;
	case OSSUARY: say("Bones, stacked like firewood.\nAt the far end, a skeleton in a wedding dress."); break;
	default: break;
	}
}

void Game::spawnEnemies() {
	easyCount = mediumCount = 0; bossActive = false;
	if (room == NURSERY) {
		sf::Vector2i e[3] = { {196, 452}, {196, 324}, {292, 228} }, m[3] = { {292, 228}, {676, 164}, {516, 128} };
		for (int i = 0; i < 3; ++i) { ghostEasy[i].setPosition({ (float)(e[i].x), (float)(e[i].y) }); ghostEasy[i].initialize(); ghostMedium[i].setPosition({ (float)(m[i].x), (float)(m[i].y) }); ghostMedium[i].initialize(); }
		easyCount = mediumCount = 3;
	}
	else if (room == CHAPEL) {
		sf::Vector2i e[3] = { {356, 388}, {324, 516}, {420, 260} }, m[2] = { {740, 132}, {388, 132} };
		for (int i = 0; i < 3; ++i) { ghostEasy[i].setPosition({ (float)(e[i].x), (float)(e[i].y) }); ghostEasy[i].initialize(); }
		for (int i = 0; i < 2; ++i) { ghostMedium[i].setPosition({ (float)(m[i].x), (float)(m[i].y) }); ghostMedium[i].initialize(); }
		easyCount = 3; mediumCount = 2;
	}
	else if (room == CRIMSON_CRYPT) { boss.setPosition({ (float)(838), (float)(228) }); bossActive = true; }

	bride.reset(ROOMS[room].wraith, frand(10, 18));
	bride.pacified = player.has(CRIMSON_LOCKET);
	bool crawl = room == NURSERY || room == HIDDEN_TUNNEL || room == OSSUARY || room == COLLAPSED_BEDROOM;
	sf::Vector2f spot = room == NURSERY ? sf::Vector2f(780, 520) : room == HIDDEN_TUNNEL ? sf::Vector2f(700, 180) :
		room == OSSUARY ? sf::Vector2f(250, 330) : sf::Vector2f(620, 560);
	child.reset(crawl, spot);
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
	bool overlay = tipOpen >= 0 || journalPage >= 0;
	if (!overlay) {
		runStats.time += dt;
		roomTime += dt;
		if (player.invulnerable > 0) player.invulnerable -= dt;
		if (flareCooldown > 0) flareCooldown -= dt;
		if (flare > 0) flare -= dt * 2.f;
		if (shake > 0) shake -= dt * 2.5f;
		if (hurtFlash > 0) hurtFlash -= dt * 1.5f;
		if (toastT > 0) toastT -= dt;
		if (eventCaptionT > 0) eventCaptionT -= dt;
		if (wallT > 0) wallT -= dt;
		if (ghostStun > 0) ghostStun -= dt;
		if (bossStun > 0) bossStun -= dt;
		if (jumpscare > 0) { jumpscare -= dt; player.frozen = jumpscare > 0.25f; }
		player.light.flicker *= 0.92f;
	}
	player.frozen = overlay || jumpscare > 0.25f;

	// Camera shake.
	if (shake > 0) {
		sf::View v = player.view;
		v.move({ (float)(frand(-4, 4) * shake), (float)(frand(-4, 4) * shake) });
		window->setView(v);
	}

	maps[room].drawMap(window);
	drawWorldOverlays(window);   // hint trail, target marker, blood writing

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
		if (overlay || bossStun > 0) { window->draw(boss.characterImage); boss.beep_sound.stop(); }
		else boss.movement(&player, window);
	}
	if (!overlay) child.update(dt, pp(), player.source.y);
	child.draw(window);

	// --- player + lantern darkness ---
	if (shake > 0) player.view.move({ (float)(0), (float)(0) });
	player.movement(window);
	if (shake > 0) {
		sf::View v = player.view; v.move({ (float)(frand(-4, 4) * shake), (float)(frand(-4, 4) * shake) }); window->setView(v);
	}

	// --- horrors that glow through the dark ---
	if (!overlay) {
		bride.update(dt, pp(), player.isMoving());
		apparition.update(dt, pp());
	}
	bride.draw(window);
	apparition.draw(window);

	if (!overlay) {
		// Story / interaction.
		if (key_hit(sf::Keyboard::Key::Space)) dialogue = "";
		if (!talkedToAlfred && (key_hit(sf::Keyboard::Key::Enter) || key_hit(sf::Keyboard::Key::E))) interact();
		if (key_hit(sf::Keyboard::Key::B)) goBack();
		if (key_hit(sf::Keyboard::Key::F)) flareLantern();
		if (key_hit(sf::Keyboard::Key::H)) { if (objectiveTime < 61) objectiveTime = 61; }
		if (key_hit(sf::Keyboard::Key::I)) showInventory = !showInventory;
		if (key_hit(sf::Keyboard::Key::Tab)) showDebug = !showDebug;
		if (key_hit(sf::Keyboard::Key::J)) journalPage = 0;

		updateHazards();
		updateDig(dt);
		updateEvents(dt);
		updateThreats(dt);
		updateHints(dt);
		if (bossActive) for (int i = 0; i < 1; ++i) {
			if (bossStun <= 0 && boss.isCollisionCharacter(&player)) { die("The Devil took you.", TIP_DEVIL); return; }
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

void Game::pickUp(int item, const std::string& msg, bool clearTile) {
	player.object_sound.play();
	ItemRecord* rec = itemDB.search(item);
	if (rec && !rec->collected) { rec->collected = true; runStats.itemsFound++; }
	if (ITEMS[item].isClue) { if (rec) rec->discovered = true; }
	player.give(item);
	showToast(std::string("+ ") + ITEMS[item].name);
	say(msg);
	if (clearTile) changeFacingTile(sf::Vector2i(-1, -1));
}

void Game::interact() {
	int id = player.col.isInteractionMap(pp(), player.source.y);
	if (id < 0) return;
	switch (id) {
	case 0: // drawer
		if (room == LIBRARY && !collected(RUSTY_KEY)) pickUp(RUSTY_KEY, "Drawer:\n\tA rusty key, wrapped in a child's ribbon.", false);
		else if (room == GRAND_CORRIDOR && !collected(NEWSPAPER))
			pickUp(NEWSPAPER, "A yellowed newspaper - June 26, 1953:\n\tA man killed ten innocent people, women and children,\n\tthen took his own life. His bride was never found.\n\tThe Mansion has never found peace.", false);
		else if (std::rand() % 3 == 0) {
			say("A cold hand closes around yours from inside the drawer!");
			player.fear += 8; audio.play(SFX_STINGER, 80); shake = 0.6f;
		}
		else say("The drawer is empty. Something scratched 'SLEEP' inside it.");
		break;
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
			audio.play(SFX_BELLS, 80);
		}
		else if (room == CRIMSON_CRYPT) {
			if (player.has(SWORD)) { player.end_sound.play(); finish(player.has(CRIMSON_LOCKET) ? END_TRUE : END_ESCAPE); }
			else say("The corpse twitches. You need a blessed blade.");
		}
		else say("Bones. Just bones. Probably.");
		break;
	case 15:
		if (!player.is_mattock) { player.object_sound.play(); changeFacingTile(sf::Vector2i(-1, -1)); player.is_mattock = true; player.give(MATTOCK); say("Picked up a mattock.\nStand still on the cracked ground to dig."); }
		else say("I already have this tool.");
		break;
	case 16: pickUp(SWORD, "The Blessed Sword.\nThe crypt door will open for you now.", true); break;
	case 17:
		player.object_sound.play();
		say("Lever pulled! The spikes sink into the floor.");
		for (int r = 0; r < GRID_H; r++) for (int c = 0; c < GRID_W; c++) if (maps[room].map.cell[r][c].x == 21) maps[room].map.cell[r][c] = sf::Vector2i(21, 0);
		changeFacingTile(sf::Vector2i(17, 2));
		break;
	case 18: if (room == CHAPEL) pickUp(IRON_KEY, "The Iron Key. It opens the Ritual Antechamber.", true); break;
	case 19:
		player.object_sound.play();
		if (room == ANTECHAMBER) say("Choose wisely - your choice decides how this ends.");
		else if (room == CRIMSON_CRYPT) say("The Devil's corpse is buried in the center.\nTake the digging tool and stand still to dig.");
		else if (room == OSSUARY) say("Carved in the stone:\n'Here lies the bride of June 26, 1953.\nReturn her heart to the light and she will rest.'");
		else say("A faint whisper: 'Don't look behind you.'");
		if (room != OSSUARY) changeFacingTile(sf::Vector2i(-1, -1));
		break;
	case 20:
		player.object_sound.play();
		say("You pour the petrol and strike a match...");
		finish(END_TRAPPED);
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
		if (player.has(CRIMSON_LOCKET)) { player.door_sound.play(); finish(END_SECRET); }
		else say("The front door won't open.\nThe house is not done with you.");
		return;
	}
	if (e->locked) {
		if (!edgeUsable(*e)) {
			if (room == ENTRANCE_HALL) say("It's about to be dark.\nTake the lantern before going any further.");
			else if (room == ANTECHAMBER) say("Make your decision before proceeding.");
			else if (e->requiredItem == NEWSPAPER) say("The door is engraved with a date, half scratched out: 'June 2_, 19_3'.\nSomewhere in this house is a record of that day.");
			else say(std::string("Locked. It needs the ") + ITEMS[e->requiredItem].name + ".");
			return;
		}
		int req = e->requiredItem;
		house.unlock(room, e->to);
		bool isKey = req == RUSTY_KEY || req == BRASS_KEY || req == CHAPEL_KEY || req == IRON_KEY;
		if (isKey) { player.take(req); showToast(std::string("- ") + ITEMS[req].name + " (used)"); }
	}
	player.door_sound.play();
	enterRoom(e->to, room, true);
}

void Game::goBack() {
	// Stack: top is the current room; the one beneath it is where we came from.
	int current, previous;
	if (!player.history.pop(&current)) return;
	if (!player.history.top(&previous)) { player.history.push(current); say("There is nowhere to go back to."); return; }
	Edge* e = house.edgeTo(room, previous);
	if (!e || !edgeUsable(*e)) { player.history.push(current); say("The way back is sealed."); return; }
	if (room == CRIMSON_CRYPT) { player.history.push(current); say("You can't leave. Not now."); return; }
	player.door_sound.play();
	enterRoom(previous, room, false);
	showToast(std::string("Went back to ") + ROOMS[previous].name);
}

void Game::flareLantern() {
	if (!player.has(LANTERN)) { say("You have nothing to light."); return; }
	if (flareCooldown > 0) { showToast("The lantern is still recovering..."); return; }
	flareCooldown = 10.f; flare = 1.f;
	player.fear -= 10;
	audio.play(SFX_BELLS, 45, 1.6f);
	sf::Vector2f c = pp() + sf::Vector2f(12, 12);
	if (bride.visible() && bride.distanceTo(pp()) < 130) {
		bride.banish(frand(25, 35)); runStats.banished++;
		showToast("The Weeping Bride shrieks and dissolves!");
		audio.play(SFX_SCREAM, 70, 1.3f);
	}
	if (child.active && child.distanceTo(pp()) < 130) {
		child.stun = 4.f; child.pos = child.home; runStats.banished++;
		showToast("The Hollow Child scuttles back into the dark.");
	}
	for (int i = 0; i < easyCount; ++i) if (dist(ghostEasy[i].characterImage.getPosition(), pp()) < 110) ghostStun = 3.5f;
	for (int i = 0; i < mediumCount; ++i) if (dist(ghostMedium[i].characterImage.getPosition(), pp()) < 110) ghostStun = 3.5f;
	if (bossActive && dist(boss.characterImage.getPosition(), pp()) < 110) { bossStun = 2.f; showToast("The Devil recoils from the light!"); }
	(void)c;
}

void Game::finish(int ending) {
	ending_type = ending;
	game_state = ST_ENDING;
	boss.beep_sound.stop();
}

// ---------------------------------------------------------------- hazards
void Game::hurt(float h, float f, const std::string& why, int tip, bool respawn) {
	if (player.invulnerable > 0) return;
	player.health -= h; player.fear += f;
	player.invulnerable = 2.f; hurtFlash = 1.f; shake = 0.8f;
	showToast(why);
	openTip(tip);
	if (player.health <= 0) { die(why, tip); return; }
	if (respawn) placePlayer(entryTile);
}

void Game::die(const std::string& why, int tip) {
	player.dead_sound.play();
	boss.beep_sound.stop();
	runStats.deaths++;
	deathReason = why; deathTip = tip;
	game_state = ST_RETRY;
}

void Game::updateHazards() {
	if (!player.isMoving()) return;
	if (!player.col.isCollisionMap(pp(), player.source.y)) return;
	sf::Vector2i a = current_map->at(player.col.next_tile1.x, player.col.next_tile1.y);
	sf::Vector2i b = current_map->at(player.col.next_tile2.x, player.col.next_tile2.y);
	auto deadly = [](sf::Vector2i t) { return t.y == 1 && (t.x == 14 || t.x == 21 || t.x == 22); };
	if (deadly(a) || deadly(b)) hurt(25, 10, "The house bites back - spikes, fire, an open grave.", TIP_HAZARDS, true);
}

void Game::updateDig(float dt) {
	if (room != CRIMSON_CRYPT) return;
	int id = player.col.isInteractionMap(pp(), player.source.y);
	if (id != 13) { digT = 0; return; }
	if (!player.is_mattock) { if (dialogue.empty()) say("Find something to dig with - and stand still!"); return; }
	if (player.isMoving()) { digT = 0; return; }
	digT += dt;
	if (digT > 3.5f) {
		digT = 0; digCount++;
		audio.play(SFX_CRACK, 100, 0.8f); shake = 0.5f;
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
		case EV_FLICKER: player.light.flicker = 1.f; eventCaption = "The lantern gutters..."; player.fear += 4; break;
		case EV_WHISPER: audio.play(SFX_WHISPER, 85); eventCaption = "...stay... sleep with us..."; player.fear += 5; break;
		case EV_SLAM: audio.play(SFX_SLAM, 90); shake = 1.f; eventCaption = "A door slams somewhere above."; player.fear += 6; break;
		case EV_BREATH: audio.play(SFX_BREATH, 70); eventCaption = "Cold breath on the back of your neck."; player.fear += 5; break;
		case EV_BLOODWRITE: {
			const char* words[] = { "GET OUT", "SHE IS STILL HERE", "1953", "SLEEP", "BEHIND YOU" };
			wallText = words[std::rand() % 5]; wallT = 7.f;
			wallPos = pp() + sf::Vector2f(frand(-90, 60), frand(-80, -40));
			audio.play(SFX_STINGER, 60); eventCaption = "Something is written on the wall - in blood."; player.fear += 7;
			break;
		}
		case EV_APPARITION: {
			float a = frand(0, 6.283f);
			apparition.spawn(pp() + sf::Vector2f(std::cos(a), std::sin(a)) * 105.f, std::rand() % 2);
			audio.play(SFX_STINGER, 75, 0.8f); eventCaption = "Someone is standing at the edge of the light."; player.fear += 8;
			break;
		}
		case EV_CRACK: audio.play(SFX_CRACK, 90, 0.9f); shake = 0.4f; eventCaption = "The floorboards crack right behind you."; player.fear += 5; break;
		default: audio.play(SFX_BELLS, 55); eventCaption = "Distant bells toll for the dead of 1953."; player.fear += 3; break;
		}
		player.fear += 3 * danger;
		eventCaptionT = 4.f;
	}
}

void Game::updateThreats(float dt) {
	threats.clear();
	sf::Vector2f p = pp();
	// Insert every active danger with its priority...
	if (bride.active && bride.visible() && !bride.pacified) {
		float d = bride.distanceTo(p);
		if (d < 22) threats.insert(Threat{ 100, TH_BRIDE_ATTACK, "THE WEEPING BRIDE HAS YOU" });
		else if (d < 240) threats.insert(Threat{ 75, TH_BRIDE_NEAR, "The Weeping Bride drifts toward you - stand still, or FLARE (F)" });
	}
	if (bossActive && bossStun <= 0 && dist(boss.characterImage.getPosition(), p) < 150) threats.insert(Threat{ 90, TH_DEVIL, "The Devil is close - the EMF meter screams" });
	if (child.active && child.stun <= 0 && child.distanceTo(p) < 170) {
		threats.insert(Threat{ child.frozen ? 50 : 80, TH_CHILD, child.frozen ? "The Hollow Child is frozen while you watch it" : "Something crawls behind you - TURN AND FACE IT" });
		openTip(TIP_CHILD);
	}
	for (int i = 0; i < easyCount; ++i) if (dist(ghostEasy[i].characterImage.getPosition(), p) < 90) threats.insert(Threat{ 60, TH_GHOST, "A restless spirit walks its old path" });
	for (int i = 0; i < mediumCount; ++i) if (dist(ghostMedium[i].characterImage.getPosition(), p) < 90) threats.insert(Threat{ 60, TH_GHOST, "A restless spirit walks its old path" });
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
	case TH_DEVIL: rate = 8; break;
	case TH_CHILD: rate = child.frozen ? 2 : 7; break;
	case TH_BRIDE_NEAR: rate = 5; break;
	case TH_GHOST: rate = 3; break;
	case TH_DARK: rate = 2; break;
	default: rate = (ROOMS[room].danger <= 3 ? -2.5f : -0.8f); break;
	}
	player.fear += rate * dt;

	if (topThreat.kind == TH_BRIDE_NEAR) openTip(TIP_BRIDE);
	if (topThreat.kind == TH_GHOST) openTip(TIP_SPIRITS);
	if (topThreat.kind == TH_DEVIL) openTip(TIP_DEVIL);
	if (topThreat.kind == TH_BRIDE_ATTACK && player.invulnerable <= 0) {
		jumpscare = 1.3f;
		audio.play(SFX_JUMPSCARE, 100);
		bride.banish(frand(25, 35));
		hurt(30, 30, "The Weeping Bride's face fills your vision.", TIP_BRIDE, false);
	}
	// Contact with patrol spirits / crawler.
	for (int i = 0; i < easyCount && ghostStun <= 0; ++i) if (ghostEasy[i].isCollisionCharacter(&player)) hurt(34, 20, "A spirit passes through you. Ice in your veins.", TIP_SPIRITS, true);
	for (int i = 0; i < mediumCount && ghostStun <= 0; ++i) if (ghostMedium[i].isCollisionCharacter(&player)) hurt(34, 20, "A spirit passes through you. Ice in your veins.", TIP_SPIRITS, true);
	if (child.active && child.stun <= 0 && child.distanceTo(p) < 18) {
		audio.play(SFX_SCREAM, 90, 1.4f);
		child.stun = 3; child.pos = child.home;
		hurt(25, 20, "Tiny teeth sink into your ankle!", TIP_CHILD, false);
	}

	if (player.fear < 0) player.fear = 0;
	if (player.fear > runStats.peakFear) runStats.peakFear = player.fear;
	if (player.fear > 55 && !tipSeen[TIP_FEAR]) openTip(TIP_FEAR);
	if (player.fear >= 100) { player.fear = 100; die("Your mind shattered. You laugh, and cannot stop.", TIP_FEAR); return; }

	// Heartbeat speeds up with fear.
	audio.setTension(player.fear / 100.f);
	heartbeatT -= dt;
	if (player.fear > 50 && heartbeatT <= 0) {
		audio.play(SFX_HEARTBEAT, 40 + player.fear * 0.5f);
		heartbeatT = 1.6f - player.fear / 100.f;
	}
}

void Game::openTip(int tip, bool force) {
	if (!force && tipSeen[tip]) return;
	tipSeen[tip] = true;
	tipOpen = tip;
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
	std::string what;
	for (int i = 0; i < 6; ++i) {
		const Step& s = STEPS[i];
		Edge* e = house.edgeTo(s.lockFrom, s.lockTo);
		if (!collected(s.item)) { key = i * 2; targetRoom = s.room; targetTile = s.tile; what = std::string("The ") + ITEMS[s.item].name + " is somewhere in the " + ROOMS[s.room].name + "."; break; }
		if (e && e->locked) { key = i * 2 + 1; targetRoom = s.lockFrom; targetTile = 3 + e->doorNo; what = std::string("Use the ") + ITEMS[s.item].name + " on the door to the " + ROOMS[s.lockTo].name + "."; break; }
	}
	if (key == 100) {
		targetRoom = CRIMSON_CRYPT;
		if (digCount >= 4) { key = 20; targetTile = 12; what = "Strike the Devil's corpse with the Blessed Sword."; }
		else if (!player.is_mattock) { key = 21 + digCount; targetTile = 15; what = "Find a mattock in the crypt to dig with."; }
		else { key = 30 + digCount; targetTile = 13; what = "Stand still on the cracked ground at the crypt's heart and dig."; }
	}
	if (key != objectiveKey) { objectiveKey = key; objectiveTime = forceHint ? 125.f : 0.f; forceHint = false; hintText = ""; trailLen = 0; }
	objectiveTime += dt;
	objectiveText = what;

	// After a minute without progress, the house "remembers" the way for you.
	if (objectiveTime >= 60) {
		std::string route;
		if (targetRoom != room) {
			lastRouteLen = house.bfsPath(room, targetRoom, lastRoute, MAX_ROOMS, [this](const Edge& e) { return edgeUsable(e); });
			if (lastRouteLen > 1) {
				route = "  Route: ";
				for (int i = 0; i < lastRouteLen; ++i) { route += ROOMS[lastRoute[i]].name; if (i + 1 < lastRouteLen) route += " > "; }
			}
		}
		else route = "  Follow the trail of blood.";
		hintText = "HINT: " + what + route;
		trailT -= dt;
		if (room == targetRoom && trailT <= 0) { computeTrail(); trailT = 0.25f; }
		if (room != targetRoom) trailLen = 0;
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
			talked = true;
			say("Alfred:\n\tYou came... thank God.\n\tThe house has slept in crimson since 1953.\n\tYou are our only hope.");
		}
		if (alfredT >= 0) {
			float before = alfredT;
			alfredT += dt;
			auto crossed = [&](float t) { return before < t && alfredT >= t; };
			if (crossed(5.f)) {
				// He hands you the lantern himself.
				if (!collected(LANTERN)) {
					pickUp(LANTERN, "", false);
					int c, r;
					if (maps[ENTRANCE_HALL].map.find(11, &c, &r)) maps[ENTRANCE_HALL].map.set(c, r, sf::Vector2i(-1, -1));
				}
				say("Alfred:\n\tTake my lantern. The lines were cut.\n\tPress F to flare it - the dead hate the light.");
			}
			if (crossed(11.f))
				say("Alfred:\n\tE to open doors, drawers and chests.\n\tB takes you back the way you came.\n\tIf you are lost, press H - the house remembers the way.");
			if (crossed(18.f)) {
				say("Alfred:\n\tIt's getting dark. I... I should go.");
				npc.state = Npc::moving;
				alfredT = -1;
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
				player.fear += 8;
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
			audio.play(SFX_STINGER, 100, 0.7f);
			shake = 1.f; hurtFlash = 0.5f;
			player.fear += 12;
			eventCaption = "Alfred turns around. His eyes are gone.";
			eventCaptionT = 5.f;
		}
	}
	else if (alfredDead == 2) {
		if (!overlay) alfredDeadT += dt;
		int frame = (int)(alfredDeadT * 8) % 3;
		alfredDeadSprite.setTextureRect(sf::IntRect({ frame * 24, 0 }, { 24, 48 }));   // facing you
		float a = alfredDeadT < 1.4f ? 1.f : std::max(0.f, 1.f - (alfredDeadT - 1.4f) / 0.6f);
		bool flick = alfredDeadT > 1.0f && std::rand() % 3 == 0;
		alfredDeadSprite.setColor(sf::Color(255, 255, 255, (std::uint8_t)(flick ? 60 : 255 * a)));
		if (alfredDeadT > 2.0f) alfredDead = 3;
	}
	window->draw(alfredDeadSprite);
}
