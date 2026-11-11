#include <algorithm>
// Graphics & UI handler: translates the data structures into pixels.
#include "game.h"
#include <cmath>
#include <cstdio>

namespace {
const sf::Color CRIMSON(176, 30, 58), BLOOD(120, 6, 18), BONE(232, 220, 200), ASH(150, 140, 136), INK(8, 2, 4);

sf::Text label(const sf::Font& f, const std::string& s, unsigned size, sf::Color c, float x, float y) {
	sf::Text t(f, s, size);
	t.setFillColor(c);
	t.setPosition({ (float)(std::floor(x)), (float)(std::floor(y)) });
	return t;
}
void panel(sf::RenderWindow* w, float x, float y, float wd, float h, std::uint8_t alpha = 215) {
	sf::RectangleShape r(sf::Vector2f(wd, h));
	r.setPosition({ (float)(x), (float)(y) });
	r.setFillColor(sf::Color(10, 2, 4, alpha));
	r.setOutlineColor(BLOOD);
	r.setOutlineThickness(2);
	w->draw(r);
}
void bar(sf::RenderWindow* w, const sf::Font& f, const char* name, float x, float y, float v, sf::Color c) {
	sf::RectangleShape back(sf::Vector2f(180, 12)); back.setPosition({ (float)(x + 70), (float)(y + 6) });
	back.setFillColor(sf::Color(30, 8, 12, 220)); back.setOutlineColor(sf::Color(70, 14, 22)); back.setOutlineThickness(1);
	sf::RectangleShape fill(sf::Vector2f(180 * (v < 0 ? 0 : v > 100 ? 1 : v / 100.f), 12)); fill.setPosition({ (float)(x + 70), (float)(y + 6) }); fill.setFillColor(c);
	w->draw(back); w->draw(fill);
	w->draw(label(f, name, 18, BONE, x, y));
}
}

void Game::drawWorldOverlays(sf::RenderWindow* w) {
	float t = runStats.time;
	// BFS hint trail: dripping blood drops fixed to every third tile of the
	// path (trail[0] is beside the goal), so they never slide as you walk.
	// Only the next few ahead of you are shown.
	{
		int pc = (int)((pp().x + 12) / 32), pr = (int)((pp().y + 12) / 32), here = trailLen - 1, best = 999;
		for (int i = 0; i < trailLen; ++i) {
			int d = std::abs(trail[i] % GRID_W - pc) + std::abs(trail[i] / GRID_W - pr);
			if (d < best) { best = d; here = i; }
		}
		int shown = 0;
		for (int i = here - 1; i >= 1 && shown < 4; --i) {
			if (i % 3 != 0) continue;
			int c = trail[i] % GRID_W, r = trail[i] / GRID_W;
			int h = (c * 73856093) ^ (r * 19349663);
			float jx = (float)(((h >> 6) & 7) - 3), jy = (float)(((h >> 9) & 7) - 3);
			decalSprite.setTextureRect(sf::IntRect({ 16, 0 }, { 16, 16 }));   // the drip
			decalSprite.setPosition({ c * 32 + 8.f + jx, r * 32 + 8.f + jy });
			decalSprite.setColor(sf::Color(255, 255, 255, (std::uint8_t)(255 * (1.f - 0.15f * shown))));
			w->draw(decalSprite);
			++shown;
		}
	}
	// After a minute the objective itself starts to pulse.
	int tc, tr;
	if (objectiveTime >= 60 && room == targetRoom && targetTile >= 0 && current_map->find(targetTile, &tc, &tr)) {
		float s = 0.5f + 0.5f * std::sin(t * 5);
		sf::RectangleShape m(sf::Vector2f(32, 32));
		m.setPosition({ (float)(tc * 32.f), (float)(tr * 32.f) });
		m.setFillColor(sf::Color(220, 20, 50, (std::uint8_t)(40 + 60 * s)));
		m.setOutlineColor(sf::Color(255, 60, 80, (std::uint8_t)(140 + 100 * s)));
		m.setOutlineThickness(2);
		w->draw(m);
	}
	// Words written in blood on the wall (they drip as they fade).
	if (wallT > 0) {
		// Scrawled onto the wall face letter by letter: each letter tilts,
		// bobs and changes size a little (seeded by its place, so it holds
		// still), with a darker smear underneath - written by a hand, not typed.
		float a = wallT > 6 ? (7 - wallT) : wallT / 6;
		float x = std::floor(wallPos.x);
		unsigned seed = (unsigned)(wallPos.x * 13 + wallPos.y * 7);
		for (size_t k = 0; k < wallText.size(); ++k) {
			unsigned h = seed * 2654435761u + (unsigned)k * 40503u;
			float tilt = (float)((int)(h % 25) - 12);                    // -12 .. 12 degrees
			float bob = (float)((int)((h >> 5) % 5) - 2);                 // -2 .. 2 px
			unsigned size = WALL_TEXT_SIZE + (unsigned)((h >> 9) % 4) - 1;  // a bit uneven
			sf::Text letter(fontBlood, std::string(1, wallText[k]), size);
			sf::FloatRect lb = letter.getLocalBounds();
			letter.setOrigin({ lb.position.x + lb.size.x / 2, lb.position.y + lb.size.y / 2 });
			letter.setRotation(sf::degrees(tilt));
			sf::Vector2f c(x + lb.size.x / 2, std::floor(wallPos.y + 11 + bob));
			letter.setPosition(c + sf::Vector2f(1.f, 1.5f));               // smear
			letter.setFillColor(sf::Color(70, 0, 8, (std::uint8_t)(170 * a)));
			w->draw(letter);
			letter.setPosition(c);
			letter.setFillColor(sf::Color(160, 4, 20, (std::uint8_t)(240 * a)));
			w->draw(letter);
			x += fontBlood.getGlyph((unsigned char)wallText[k], WALL_TEXT_SIZE, false).advance + (wallText[k] == ' ' ? 7.f : 1.f);
		}
	}
}

void Game::drawHud(sf::RenderWindow* w) {
	w->setView(uiView(*w));
	const float W = 1024, H = 768;
	float t = runStats.time;

	// Fear vignette: the edges of the screen bleed as fear rises.
	float f = player.fear / 100.f;
	for (int i = 0; i < 6; ++i) {
		float inset = i * 14.f;
		sf::RectangleShape v(sf::Vector2f(W - 2 * inset, H - 2 * inset));
		v.setPosition({ (float)(inset), (float)(inset) });
		v.setFillColor(sf::Color::Transparent);
		v.setOutlineColor(sf::Color(110, 0, 10, (std::uint8_t)(f * (60 - i * 9))));
		v.setOutlineThickness(14);
		w->draw(v);
	}
	if (hurtFlash > 0) { sf::RectangleShape r(sf::Vector2f(W, H)); r.setFillColor(sf::Color(160, 0, 10, (std::uint8_t)(120 * hurtFlash))); w->draw(r); }
	if (flare > 0) { sf::RectangleShape r(sf::Vector2f(W, H)); r.setFillColor(sf::Color(255, 210, 190, (std::uint8_t)(150 * flare))); w->draw(r); }

	// Status panel.
	panel(w, 12, 12, 270, 96);
	bar(w, fontSerif, "Health", 22, 18, player.health, sf::Color(170, 16, 30));
	bar(w, fontSerif, "Fear", 22, 44, player.fear, sf::Color(110, 60, 160));
	char buf[64];
	// Flare recharge: a bar that refills, with the seconds left on it.
	if (!player.has(LANTERN)) w->draw(label(fontSerif, "No light", 17, ASH, 22, 72));
	else {
		float ready = 1.f - flareCooldown / FLARE_COOLDOWN;
		ready = std::max(0.f, std::min(1.f, ready));
		bool full = flareCooldown <= 0;
		float glow = full ? 0.75f + 0.25f * std::sin(t * 4) : 1.f;
		sf::Color fillCol = full ? sf::Color((std::uint8_t)(255 * glow), (std::uint8_t)(200 * glow), (std::uint8_t)(110 * glow)) : sf::Color(150, 110, 60);
		bar(w, fontSerif, "Flare", 22, 70, ready * 100.f, fillCol);
		if (full) std::snprintf(buf, sizeof buf, "ready - press F");
		else std::snprintf(buf, sizeof buf, "%.0fs", std::ceil(flareCooldown));
		sf::Text tt(fontMono, buf, 11);
		tt.setFillColor(full ? sf::Color(40, 10, 4) : BONE);
		// centre on the bar (x 92..272, y 76..88) using the text's real bounds
		sf::FloatRect lb = tt.getLocalBounds();
		tt.setOrigin({ lb.position.x + lb.size.x / 2.f, lb.position.y + lb.size.y / 2.f });
		tt.setPosition({ 92.f + 90.f, 76.f + 6.f });
		w->draw(tt);
	}

	// "[E] Talk" over Alfred's head when you're next to him.
	if (room == ENTRANCE_HALL && npc.state == Npc::standing && npc.near(&player)) {
		sf::Vector2i px = w->mapCoordsToPixel(npc.characterImage.getPosition() + sf::Vector2f(12.f, -30.f), player.view);
		sf::Vector2f sp = w->mapPixelToCoords(px, uiView(*w));
		sf::Text tt(fontSerif, "[E] Talk to Alfred", 20);
		tt.setFillColor(BONE);
		tt.setOutlineColor(sf::Color(0, 0, 0)); tt.setOutlineThickness(2);
		tt.setPosition({ std::floor(sp.x - tt.getLocalBounds().size.x / 2), std::floor(sp.y) });
		w->draw(tt);
	}
	// Room name.
	sf::Text rn(fontTitle, ROOMS[room].name, 30);
	rn.setFillColor(CRIMSON);
	rn.setPosition({ (float)(W - rn.getLocalBounds().size.x - 24), (float)(10) });
	w->draw(rn);

	// Messages sit below the mini-map (which ends at y ~126) with a dark outline.
	// Top threat from the Priority Queue.
	if (topThreat.kind != TH_CALM && topThreat.label[0]) {
		sf::Text th(fontSerif, topThreat.label, 22);
		float pulse = topThreat.priority >= 75 ? 0.6f + 0.4f * std::sin(t * 10) : 1.f;
		th.setFillColor(sf::Color(230, 40, 60, (std::uint8_t)(255 * pulse)));
		th.setOutlineColor(sf::Color(0, 0, 0, 220)); th.setOutlineThickness(2);
		th.setPosition({ (float)(std::floor((W - th.getLocalBounds().size.x) / 2)), (float)(164) });
		w->draw(th);
	}
	if (eventCaptionT > 0) {
		sf::Text ec(fontItalic, eventCaption, 22);
		ec.setFillColor(sf::Color(200, 170, 165, (std::uint8_t)(255 * (eventCaptionT > 1 ? 1 : eventCaptionT))));
		ec.setOutlineColor(sf::Color(0, 0, 0, 220)); ec.setOutlineThickness(2);
		ec.setPosition({ (float)(std::floor((W - ec.getLocalBounds().size.x) / 2)), (float)(192) });
		w->draw(ec);
	}
	if (toastT > 0) {
		sf::Text tt(fontSerif, toast, 22);
		tt.setFillColor(sf::Color(240, 200, 120, (std::uint8_t)(255 * (toastT > 1 ? 1 : toastT))));
		tt.setOutlineColor(sf::Color(0, 0, 0, 220)); tt.setOutlineThickness(2);
		tt.setPosition({ (float)(std::floor((W - tt.getLocalBounds().size.x) / 2)), (float)(136) });
		w->draw(tt);
	}

	// Hint (after 60 s) or the plain objective.
	if (!hintText.empty()) {
		panel(w, 12, H - 92, W - 24, 34, 200);
		w->draw(label(fontSerif, hintText, 19, sf::Color(255, 120, 130), 22, H - 89));
	}

	// Demo banner.
	if (demo) {
		sf::Text dm(fontSerif, "WATCHING THE DREAM  -  a full playthrough  -  ESC to stop", 18);
		dm.setFillColor(sf::Color(255, 200, 200, (std::uint8_t)(170 + 60 * std::sin(t * 2.f))));
		dm.setOutlineColor(sf::Color::Black); dm.setOutlineThickness(2);
		dm.setPosition({ std::floor((W - dm.getLocalBounds().size.x) / 2), H - 56.f });
		w->draw(dm);
	}

	// Controls strip.
	w->draw(label(fontSerif, "E interact   F flare   B go back   H hint   I items   J tips   TAB data structures   M mute   ESC pause", 16, ASH, 22, H - 26));

	// Story text box.
	if (!dialogue.empty()) {
		sf::Text dt(fontSerif, dialogue, 22);
		dt.setFillColor(BONE);
		float bw = dt.getLocalBounds().size.x + 40, bh = dt.getLocalBounds().size.y + 44;
		if (bw < 420) bw = 420;
		float x = (W - bw) / 2, y = H - 110 - bh;
		panel(w, x, y, bw, bh, 225);
		dt.setPosition({ (float)(x + 20), (float)(y + 14) });
		w->draw(dt);
		if (alfredT >= 0) w->draw(label(fontItalic, "Space: next  -  X: skip", 16, ASH, x + bw - 172, y + bh - 22));
		else w->draw(label(fontItalic, "Space", 16, ASH, x + bw - 52, y + bh - 22));
	}

	drawMinimap(w);
	if (showInventory) drawInventory(w);

	// Jump scare: the face slams in full-screen - punch zoom, violent shake,
	// red/cyan channel split, static, and a blood flash at the start.
	if (jumpscare > 0) {
		float age = 1.3f - jumpscare;                       // 0 .. 1.3 s
		sf::RectangleShape r(sf::Vector2f(W, H)); r.setFillColor(sf::Color::Black); w->draw(r);
		float punch = age < 0.12f ? 0.55f + age / 0.12f * 0.6f : 1.15f + 0.05f * std::sin(age * 40);
		float sx = W / jumpTex.getSize().x * punch, sy = H / jumpTex.getSize().y * punch;
		float jit = age < 0.6f ? 18.f : 8.f;
		sf::Vector2f c(W / 2 + (std::rand() % 21 - 10) * jit / 10, H / 2 + (std::rand() % 21 - 10) * jit / 10);
		std::uint8_t a = (std::uint8_t)(255 * (jumpscare > 0.35f ? 1 : jumpscare / 0.35f));
		jumpSprite.setScale({ (float)(sx), (float)(sy) });
		jumpSprite.setPosition({ (float)(c.x - 7), (float)(c.y) });
		jumpSprite.setColor(sf::Color(255, 0, 0, (std::uint8_t)(a * 0.3f)));
		w->draw(jumpSprite, sf::BlendAdd);
		jumpSprite.setPosition({ (float)(c.x + 7), (float)(c.y + 2) });
		jumpSprite.setColor(sf::Color(0, 120, 120, (std::uint8_t)(a * 0.2f)));
		w->draw(jumpSprite, sf::BlendAdd);
		jumpSprite.setPosition(c);
		jumpSprite.setColor(sf::Color(255, 255, 255, a));
		w->draw(jumpSprite);
		// static lines
		for (int i = 0; i < 40; ++i) {
			sf::RectangleShape ln(sf::Vector2f(W, (float)(1 + std::rand() % 3)));
			ln.setPosition({ (float)(0), (float)((float)(std::rand() % 768)) });
			ln.setFillColor(sf::Color(255, 200, 200, (std::uint8_t)((std::rand() % 50) * a / 255)));
			w->draw(ln);
		}
		if (age < 0.09f || (age > 0.2f && age < 0.25f)) {
			sf::RectangleShape redFlash(sf::Vector2f(W, H)); redFlash.setFillColor(sf::Color(200, 0, 20, 110)); w->draw(redFlash);
		}
	}
	// F9 diagnostics: what the game thinks the player is doing right now.
	if (g_diag) {
		using K = sf::Keyboard::Key;
		auto k = [](K a, K b) { return key_held(a) || key_held(b); };
		static sf::Clock fc; static int frames = 0; static float fps = 0;
		++frames; if (fc.getElapsedTime().asSeconds() >= 0.5f) { fps = frames / fc.restart().asSeconds(); frames = 0; }
		sf::Vector2f p = pp();
		char d[512];
		std::snprintf(d, sizeof d,
			"DIAG  fps %.0f   pos %.0f, %.0f   room %d\n"
			"keys  L%d R%d U%d D%d\n"
			"frozen %d  lockInput %d  autoDir %d  forcedDir %d  moving %d\n"
			"tipOpen %d  journal %d  jumpscare %.2f  alfredT %.1f\n"
			"blocked  down %d  left %d  right %d  up %d   speed %.2f",
			fps, p.x, p.y, room,
			k(K::Left, K::A), k(K::Right, K::D), k(K::Up, K::W), k(K::Down, K::S),
			(int)player.frozen, (int)player.lockInput, player.autoDir, player.forcedDir, (int)player.isMoving(),
			tipOpen, journalPage, jumpscare, alfredT,
			player.col.isCollisionMap(p, 0), player.col.isCollisionMap(p, 1), player.col.isCollisionMap(p, 2), player.col.isCollisionMap(p, 3),
			player.characterSpeed);
		sf::Text dt(fontMono, d, 14);
		dt.setFillColor(sf::Color(255, 255, 120)); dt.setOutlineColor(sf::Color::Black); dt.setOutlineThickness(2);
		dt.setPosition({ 16.f, 300.f });
		w->draw(dt);
	}
	if (tipOpen >= 0) drawTip(w, tipOpen);
	if (journalPage >= 0) drawJournal(w);
}

void Game::drawTip(sf::RenderWindow* w, int tip) {
	sf::RectangleShape dim(sf::Vector2f(1024, 768)); dim.setFillColor(sf::Color(0, 0, 0, 150)); w->draw(dim);
	panel(w, 152, 210, 720, 300, 240);
	w->draw(label(fontItalic, "Survival note", 20, ASH, 178, 222));
	w->draw(label(fontTitle, TIPS[tip].title, 40, CRIMSON, 176, 244));
	w->draw(label(fontSerif, TIPS[tip].body, 22, BONE, 178, 300));
	w->draw(label(fontItalic, "Enter to continue   -   J reopens your notes", 17, ASH, 178, 478));
}

void Game::drawJournal(sf::RenderWindow* w) {
	sf::RectangleShape dim(sf::Vector2f(1024, 768)); dim.setFillColor(sf::Color(0, 0, 0, 170)); w->draw(dim);
	panel(w, 152, 190, 720, 340, 240);
	w->draw(label(fontItalic, "Notes on the dead  (Left/Right to turn the page)", 18, ASH, 178, 200));
	bool seen = tipSeen[journalPage];
	w->draw(label(fontTitle, seen ? TIPS[journalPage].title : "? ? ?", 40, CRIMSON, 176, 228));
	w->draw(label(fontSerif, seen ? TIPS[journalPage].body : "You have not met this horror yet.", 22, BONE, 178, 286));
	char b[16]; std::snprintf(b, sizeof b, "%d / %d", journalPage + 1, (int)TIP_COUNT);
	w->draw(label(fontSerif, b, 18, ASH, 800, 500));
}

void Game::drawInventory(sf::RenderWindow* w) {
	panel(w, 12, 120, 380, 40 + 26.f * (player.inventory.size() + 1));
	w->draw(label(fontTitle, "Inventory  (Linked List)", 22, CRIMSON, 24, 124));
	float y = 156;
	if (player.inventory.empty()) w->draw(label(fontItalic, "Your pockets are empty.", 18, ASH, 28, y));
	player.inventory.forEach([&](const InventoryItem& it) {
		w->draw(label(fontSerif, std::string("- ") + ITEMS[it.id].name, 19, BONE, 28, y));
		y += 26;
	});
}

void Game::drawDebug(sf::RenderWindow* w) {
	panel(w, 520, 56, 492, 610, 235);
	float x = 532, y = 62;
	auto line = [&](const std::string& s, sf::Color c = BONE, unsigned size = 13) { w->draw(label(fontMono, s, size, c, x, y)); y += size + 5; };
	auto head = [&](const std::string& s) { y += 4; line(s, sf::Color(255, 90, 110), 14); };
	char b[160];

	head("GRAPH  adjacency list of " + std::string(ROOMS[room].name));
	for (const Edge* e = house.neighbours(room); e; e = e->next) {
		std::snprintf(b, sizeof b, "  door %d -> %-20s %s%s", e->doorNo, ROOMS[e->to].name, e->hidden ? "[hidden] " : "", e->locked ? (std::string("[locked: ") + ITEMS[e->requiredItem].name + "]").c_str() : "[open]");
		line(b);
	}
	head("STACK  movement history (top first)");
	int hist[32]; int hn = player.history.toArray(hist, 32);
	std::string s = "  ";
	for (int i = 0; i < hn && i < 6; ++i) { s += ROOMS[hist[i]].name; if (i + 1 < hn && i < 5) s += " | "; }
	if (hn > 6) s += " ...";
	line(s);
	head("QUEUE  pending haunted events (FIFO)");
	HauntEvent ev[16]; int en = events.toArray(ev, 16);
	const char* names[] = { "flicker", "whisper", "door slam", "breath", "blood writing", "moan", "crack", "glass" };
	s = "  ";
	for (int i = 0; i < en; ++i) { std::snprintf(b, sizeof b, "%s@%.0fs  ", names[ev[i].kind], ev[i].at); s += b; }
	line(en ? s : "  (empty)");
	head("PRIORITY QUEUE  active threats (max-heap)");
	for (int i = 0; i < threats.size(); ++i) {
		std::snprintf(b, sizeof b, "  [%3d] %s", threats.at(i).priority, threats.at(i).label[0] ? threats.at(i).label : "calm");
		line(std::string(b).substr(0, 62), i == 0 ? sf::Color(255, 160, 160) : BONE);
	}
	head("LINKED LIST  inventory");
	s = "  ";
	player.inventory.forEach([&](const InventoryItem& it) { s += ITEMS[it.id].name; s += " -> "; });
	line(s + "null");
	std::snprintf(b, sizeof b, "BST  item database (in-order, height %d)", itemDB.height());
	head(b);
	s = "  "; int k = 0;
	itemDB.inOrder([&](int key, const ItemRecord& r) {
		std::snprintf(b, sizeof b, "%d%s ", key, r.discovered ? "*" : r.collected ? "+" : "");
		s += b; ++k;
	});
	line(s + "  (+ found, * read)");
	head("BFS  last hint route");
	s = "  ";
	for (int i = 0; i < lastRouteLen; ++i) { s += ROOMS[lastRoute[i]].name; if (i + 1 < lastRouteLen) s += " > "; }
	line(lastRouteLen ? s : "  (no hint requested yet)");
	std::snprintf(b, sizeof b, "  tile trail: %d steps", trailLen);
	line(b);
	head("DFS  last exploration (Hymn Page)");
	s = "  ";
	for (int i = 0; i < lastDfsLen; ++i) { s += std::to_string(lastDfs[i]); if (i + 1 < lastDfsLen) s += " > "; }
	line(lastDfsLen ? s : "  (not run yet)");
	head("ARRAY  ROOMS[] / ITEMS[]");
	std::snprintf(b, sizeof b, "  ROOMS[%d] danger=%d dark=%d bride=%d", room, ROOMS[room].danger, ROOMS[room].dark, ROOMS[room].wraith);
	line(b);
}

// Mini-map of the house GRAPH: nodes are rooms, lines are edges.  Unvisited
// rooms show as '?', locked doors are dark red, secret passages stay
// invisible until the DFS reveals them, and the BFS hint route glows.
void Game::drawMinimap(sf::RenderWindow* w) {
	// Layout follows where the doors really are: Nursery above the Library,
	// Bedroom below it, Corridor to the Library's right with the Hall below,
	// the Antechamber up-right (Crypt to its left), the Chapel down-right with
	// the Hidden Tunnel and Ossuary going down beneath it.
	static const sf::Vector2i POS[ROOM_COUNT] = {
		{ -1, -1 }, { 1, 2 }, { 1, 1 }, { 0, 1 }, { 0, 2 }, { 0, 0 }, { 2, 2 }, { 2, 0 }, { 1, 0 }, { 2, 3 }, { 2, 4 } };
	static const char* SHORT[ROOM_COUNT] = { "", "Hall", "Corridor", "Library", "Bedroom", "Nursery", "Chapel", "Antechamber", "Crypt", "Tunnel", "Ossuary" };
	const float cw = 92, ch = 22, bw = 80, bh = 16;
	const float ox = (1024 - 3 * cw) / 2 + (cw - bw) / 2, oy = 10;
	panel(w, ox - 10, oy - 4, 3 * cw + 2, 5 * ch + 6, 190);
	auto box = [&](int r) { return sf::FloatRect({ ox + POS[r].x * cw, oy + POS[r].y * ch }, { bw, bh }); };
	auto known = [&](int r) {
		if (r == HIDDEN_TUNNEL || r == OSSUARY) { const Edge* e = house.edgeTo(CHAPEL, HIDDEN_TUNNEL); return e && !e->hidden; }
		return true;
	};
	// Where on the box a door is: the edge of the map its door tile sits on.
	auto anchor = [&](int r, int doorNo) {
		sf::FloatRect b = box(r);
		sf::Vector2f mid(b.position.x + bw / 2, b.position.y + bh / 2);
		int c, row;
		if (!maps[r].map.find(3 + doorNo, &c, &row)) return mid;
		if (row <= 5) return sf::Vector2f(mid.x + (c - 16) * 1.2f, b.position.y);
		if (row >= 15 || (c > 6 && c < 25)) return sf::Vector2f(mid.x + (c - 16) * 1.2f, b.position.y + bh);
		if (c <= 6) return sf::Vector2f(b.position.x, mid.y);
		return sf::Vector2f(b.position.x + bw, mid.y);
	};
	bool onRoute[MAX_ROOMS] = {};
	if (!hintText.empty()) for (int i = 0; i < lastRouteLen; ++i) onRoute[lastRoute[i]] = true;
	for (int r = 1; r < ROOM_COUNT; ++r) {
		if (!known(r)) continue;
		for (const Edge* e = house.neighbours(r); e; e = e->next) {
			if (e->to == OUTSIDE || e->to < r || e->hidden || !known(e->to)) continue;
			const Edge* back = house.edgeTo(e->to, r);
			sf::Vector2f a = anchor(r, e->doorNo), b = back ? anchor(e->to, back->doorNo) : sf::Vector2f(box(e->to).position.x + bw / 2, box(e->to).position.y + bh / 2);
			sf::Color col = e->locked ? sf::Color(150, 20, 34) : sf::Color(170, 140, 130);
			if (onRoute[r] && onRoute[e->to]) col = sf::Color(255, 120, 130);
			// elbow line: out of the door side, then across
			sf::Vector2f m = (std::fabs(a.y - b.y) > std::fabs(a.x - b.x)) ? sf::Vector2f(a.x, (a.y + b.y) / 2) : sf::Vector2f((a.x + b.x) / 2, a.y);
			sf::Vector2f m2 = (std::fabs(a.y - b.y) > std::fabs(a.x - b.x)) ? sf::Vector2f(b.x, m.y) : sf::Vector2f(m.x, b.y);
			sf::Vertex ln[6] = { { a, col }, { m, col }, { m, col }, { m2, col }, { m2, col }, { b, col } };
			w->draw(ln, 6, sf::PrimitiveType::Lines);
		}
	}
	float t = runStats.time;
	for (int r = 1; r < ROOM_COUNT; ++r) {
		if (!known(r)) continue;
		sf::FloatRect b = box(r);
		sf::RectangleShape bx({ bw, bh });
		bx.setPosition(b.position);
		bool here = r == room;
		float pulse = 0.6f + 0.4f * std::sin(t * 5);
		bx.setFillColor(here ? sf::Color((std::uint8_t)(150 * pulse), 10, 30) : roomVisited[r] ? sf::Color(40, 10, 16) : sf::Color(16, 6, 8));
		bx.setOutlineColor(onRoute[r] ? sf::Color(255, 120, 130) : r == targetRoom && objectiveTime >= 30 ? sf::Color(255, 200, 120) : sf::Color(90, 20, 30));
		bx.setOutlineThickness(1);
		w->draw(bx);
		sf::Text tx(fontMono, roomVisited[r] ? SHORT[r] : "?", 10);
		tx.setFillColor(here ? sf::Color::White : roomVisited[r] ? BONE : ASH);
		sf::FloatRect tb = tx.getLocalBounds();
		tx.setPosition({ std::floor(b.position.x + (bw - tb.size.x) / 2 - tb.position.x), std::floor(b.position.y + 2) });
		w->draw(tx);
	}
}

// The live data-structure console (its own window, opened with TAB or the
// console button).  One card per structure; each card DRAWS the structure
// (boxes, arrows, bars) with real game data instead of describing it.
void Game::drawConsole(sf::RenderWindow* w) {
	// Two views over the console window: a fixed one for the background and
	// scroll bar, and one moved down by g_consoleScroll for the cards.
	const float pw = CONSOLE_PANEL_W, PH = 768.f;   // the strip beside the game
	sf::View fixedV(sf::FloatRect({ 0.f, 0.f }, { pw, PH }));
	fixedV.setViewport(sf::FloatRect({ g_gameFrac, 0.f }, { 1.f - g_gameFrac, 1.f }));
	sf::View scrollV = fixedV;
	g_consoleScroll = std::clamp(g_consoleScroll, 0.f, std::max(0.f, g_consoleContentH - PH));
	scrollV.move({ 0.f, std::floor(g_consoleScroll) });
	w->setView(fixedV);
	const float W = pw - 10;   // leave a strip on the right for the scroll bar
	const sf::Color BG(12, 4, 6), CARD(24, 8, 12), EDGE(70, 16, 26), TITLE(236, 120, 130), TAG(120, 100, 100);
	const sf::Color OPEN(120, 200, 150), LOCK(230, 110, 110), CHIP(42, 14, 20), HI(110, 22, 36);
	sf::RectangleShape bg({ pw, PH }); bg.setFillColor(BG); w->draw(bg);
	w->setView(scrollV);

	auto txt = [&](const std::string& str, const sf::Font& f, unsigned size, sf::Color c, float x, float y, bool centre = false) {
		sf::Text t(f, sf::String::fromUtf8(str.begin(), str.end()), size); t.setFillColor(c);
		sf::FloatRect b = t.getLocalBounds();
		t.setPosition({ std::floor(centre ? x - b.size.x / 2 - b.position.x : x), std::floor(y) });
		w->draw(t);
		return b.size.x;
	};
	auto width = [&](const std::string& str, const sf::Font& f, unsigned size) {
		sf::Text t(f, sf::String::fromUtf8(str.begin(), str.end()), size); return t.getLocalBounds().size.x;
	};
	auto rect = [&](float x, float y, float wd, float h, sf::Color fill, sf::Color line = sf::Color::Transparent) {
		sf::RectangleShape r({ wd, h }); r.setPosition({ x, y }); r.setFillColor(fill);
		if (line.a) { r.setOutlineColor(line); r.setOutlineThickness(1); }
		w->draw(r);
	};
	auto lineTo = [&](sf::Vector2f a, sf::Vector2f b, sf::Color c) { sf::Vertex v[2] = { { a, c }, { b, c } }; w->draw(v, 2, sf::PrimitiveType::Lines); };
	auto arrow = [&](float x, float y, sf::Color c) {   // small right-pointing arrow
		lineTo({ x, y }, { x + 10, y }, c); lineTo({ x + 10, y }, { x + 6, y - 3 }, c); lineTo({ x + 10, y }, { x + 6, y + 3 }, c);
	};
	// a chip: rounded-looking label box; returns its width
	auto chip = [&](float x, float y, const std::string& str, sf::Color fill, sf::Color ink, sf::Color line = sf::Color::Transparent) {
		float wd = width(str, fontSerif, 14) + 14;
		rect(x, y, wd, 22, fill, line);
		txt(str, fontSerif, 14, ink, x + 7, y + 2);
		return wd;
	};
	float y = 10;
	auto card = [&](float h, const std::string& title, const std::string& tag) {
		rect(10, y, W - 20, h, CARD, EDGE);
		txt(title, fontSerif, 16, TITLE, 20, y + 4);
		float tw = width(tag, fontMono, 10);
		rect(W - 24 - tw - 8, y + 8, tw + 8, 15, sf::Color(40, 16, 22));
		txt(tag, fontMono, 10, TAG, W - 24 - tw - 4, y + 9);
		float top = y + 28; y += h + 12; return top;
	};
	char b[160];

	// ---- header: the current room + its fixed facts (ARRAY)
	txt(ROOMS[room].name, fontTitle, 26, CRIMSON, 14, y - 2);
	txt("TAB closes  -  wheel scrolls", fontItalic, 12, TAG, W - 14 - width("TAB closes  -  wheel scrolls", fontItalic, 12), y + 10);
	y += 34;
	{
		float x = 14;
		std::snprintf(b, sizeof b, "danger %d/10", ROOMS[room].danger);
		x += chip(x, y, b, CHIP, BONE) + 6;
		x += chip(x, y, ROOMS[room].dark ? "dark" : "lit", CHIP, BONE) + 6;
		x += chip(x, y, ROOMS[room].wraith ? "ghost can appear" : "no drifting ghost", CHIP, BONE) + 6;
		txt("ARRAY", fontMono, 10, TAG, x + 4, y + 6);
		y += 32;
	}

	// ---- GRAPH: this room and where its doors lead
	{
		static const char* SHORT[ROOM_COUNT] = { "", "Hall", "Corridor", "Library", "Bedroom", "Nursery", "Chapel", "Antechamber", "Crypt", "Tunnel", "Ossuary" };
		float t = card(126, "Doors from here", "GRAPH");
		float cx = W / 2;
		float cw = width(ROOMS[room].name, fontSerif, 14) + 16;
		rect(cx - cw / 2, t, cw, 24, HI, TITLE);
		txt(ROOMS[room].name, fontSerif, 14, sf::Color::White, cx, t + 3, true);
		const Edge* list[6]; int n = 0;
		for (const Edge* e = house.neighbours(room); e && n < 6; e = e->next) if (!e->hidden) list[n++] = e;
		float slot = (W - 40) / std::max(1, n);
		for (int i = 0; i < n; ++i) {
			const Edge* e = list[i];
			float nx = 20 + slot * i + slot / 2, ny = t + 46;
			std::string name = e->to == OUTSIDE ? "Outside" : SHORT[e->to];
			unsigned fs = width(name, fontSerif, 13) > slot - 10 ? 11 : 13;
			float nw = std::min(slot - 4, width(name, fontSerif, fs) + 12);
			lineTo({ cx, t + 24 }, { nx, ny }, e->locked ? LOCK : OPEN);
			rect(nx - nw / 2, ny, nw, 22, CHIP, e->locked ? LOCK : OPEN);
			txt(name, fontSerif, fs, BONE, nx, ny + 3, true);
			std::string st = e->locked ? std::string("needs ") + ITEMS[e->requiredItem].name : "open";
			float sy = ny + 25 + ((n > 3 && i % 2) ? 14 : 0);    // stagger so neighbours' labels never collide
			txt(st, fontItalic, 11, e->locked ? LOCK : OPEN, nx, sy, true);
		}
	}

	// ---- STACK: rooms visited, top = where you are
	{
		float t = card(76, "Where you've been", "STACK");
		int hist[32]; int hn = player.history.toArray(hist, 32);
		for (int i = 0; i < hn && i < 2; ++i) {
			float bx = 22, by = t + i * 22, bw = 160;
			rect(bx, by, bw, 20, i == 0 ? HI : CHIP, i == 0 ? TITLE : EDGE);
			txt(ROOMS[hist[i]].name, fontSerif, 13, i == 0 ? sf::Color::White : BONE, bx + 8, by + 2);
		}
		txt("TOP  -  you are here", fontItalic, 12, TAG, 192, t + 3);
		if (hn > 1) txt(std::string("B goes back one room"), fontItalic, 12, TAG, 192, t + 25);
		if (hn > 2) { std::snprintf(b, sizeof b, "+%d more below", hn - 2); txt(b, fontItalic, 11, TAG, W - 22 - width(b, fontItalic, 11), t + 25); }
	}

	// ---- QUEUE: scares in the order they will fire
	{
		float t = card(54, "Next scares in this room", "QUEUE");
		const char* what[] = { "flicker", "whisper", "door slam", "whisper", "blood writing", "moan", "floor crack", "glass" };
		HauntEvent ev[16]; int en = events.toArray(ev, 16);
		float x = 22;
		if (en == 0) txt("none left - the room is quiet", fontItalic, 13, TAG, 22, t + 2);
		for (int i = 0; i < en && i < 4; ++i) {
			float in = std::max(0.f, ev[i].at - roomTime);
			std::snprintf(b, sizeof b, "%s  %.0fs", what[ev[i].kind], in);
			float cw = width(b, fontSerif, 14) + 14;
			if (x + cw > W - 30) break;
			chip(x, t, b, i == 0 ? HI : CHIP, BONE, i == 0 ? TITLE : sf::Color::Transparent);
			x += cw;
			if (i + 1 < en && i < 3) { arrow(x + 4, t + 11, TAG); x += 18; }
		}
	}

	// ---- PRIORITY QUEUE: current dangers, worst first
	{
		float t = card(70, "Dangers right now", "PRIORITY QUEUE");
		int shown = 0;
		for (int i = 0; i < threats.size() && shown < 2; ++i) {
			const Threat& th = threats.at(i);
			if (th.kind == TH_CALM || !th.label[0]) continue;
			float by = t + shown * 20, maxw = W - 90;
			rect(24, by + 4, maxw, 12, CHIP);
			rect(24, by + 4, maxw * th.priority / 100.f, 12, shown == 0 ? sf::Color(200, 40, 60) : sf::Color(130, 50, 60));
			std::string lbl = th.label; if (lbl.size() > 44) lbl = lbl.substr(0, 42) + "...";
			std::snprintf(b, sizeof b, "%d", th.priority);
			txt(b, fontMono, 11, BONE, W - 56, by + 3);
			txt(lbl, fontSerif, 12, shown == 0 ? sf::Color::White : BONE, 30, by + 1);
			++shown;
		}
		if (!shown) chip(22, t, "calm - nothing is hunting you", CHIP, OPEN);
	}

	// ---- LINKED LIST: inventory
	{
		float t = card(54, "Carrying", "LINKED LIST");
		float x = 22; int k = 0;
		bool overflow = false;
		player.inventory.forEach([&](const InventoryItem& it) {
			if (overflow) return;
			float cw = width(ITEMS[it.id].name, fontSerif, 14) + 14;
			if (x + cw + 60 > W - 20) { overflow = true; return; }
			x += chip(x, t, ITEMS[it.id].name, k == 0 ? HI : CHIP, BONE);
			arrow(x + 4, t + 11, TAG); x += 18; ++k;
		});
		txt(overflow ? "..." : "null", fontMono, 12, TAG, x, t + 4);
	}

	// ---- BST: the item / clue database drawn as the actual tree.
	// x = in-order position, y = depth; lines join each node to its parent.
	// The path search() walked for the last item you picked up is lit red.
	{
		float t = card(206, "Items and clues", "BINARY SEARCH TREE");
		const int MAXN = 32;
		int key[MAXN], depth[MAXN], parent[MAXN]; ItemRecord rec[MAXN]; int n = 0;
		itemDB.walk([&](int k, const ItemRecord& r, int d, int p) {
			if (n < MAXN) { key[n] = k; rec[n] = r; depth[n] = d; parent[n] = p; ++n; }
		});
		const float R = 11.f, step = (W - 36) / std::max(1, n), ystep = 25.f;
		auto at = [&](int i) { return sf::Vector2f(18 + step * (i + 0.5f), t + R + 2 + depth[i] * ystep); };
		auto indexOf = [&](int k) { for (int i = 0; i < n; ++i) if (key[i] == k) return i; return -1; };
		int path[MAXN]; int pn = g_lastLookup >= 0 ? itemDB.pathTo(g_lastLookup, path, MAXN) : 0;
		auto onPath = [&](int k) { for (int i = 0; i < pn; ++i) if (path[i] == k) return true; return false; };
		const sf::Color GOLD(240, 200, 120), PATH(255, 70, 90);
		for (int i = 0; i < n; ++i) {
			int pi = indexOf(parent[i]);
			if (pi < 0) continue;
			bool lit = onPath(key[i]) && onPath(parent[i]);
			sf::Vector2f a = at(pi), c = at(i);
			lineTo(a, c, lit ? PATH : EDGE);
			if (lit) lineTo(a + sf::Vector2f(1, 0), c + sf::Vector2f(1, 0), PATH);
		}
		int found = 0, read = 0;
		for (int i = 0; i < n; ++i) {
			const ItemRecord& r = rec[i];
			bool clue = ITEMS[r.id].isClue;
			if (r.collected) ++found;
			if (r.discovered && clue) ++read;
			sf::Color fill = !r.collected ? CHIP : clue ? GOLD : OPEN;
			sf::CircleShape c(R); c.setOrigin({ R, R }); c.setPosition(at(i));
			c.setFillColor(fill);
			bool lit = onPath(key[i]);
			c.setOutlineColor(lit ? PATH : EDGE); c.setOutlineThickness(lit ? 2.f : 1.f);
			w->draw(c);
			txt(std::to_string(key[i]), fontMono, 11, r.collected ? sf::Color(20, 6, 8) : TAG, at(i).x, at(i).y - 7, true);
		}
		float ly = t + R + 2 + (itemDB.height() - 1) * ystep + R + 14;
		rect(18, ly - 6, W - 36, 1, EDGE);   // divider between the tree and its text
		ly += 4;
		if (pn > 0) {
			std::string s = std::string("search(") + ITEMS[g_lastLookup].name + ")  ";
			for (int i = 0; i < pn; ++i) s += std::to_string(path[i]) + (i + 1 < pn ? " > " : "");
			std::snprintf(b, sizeof b, "   %d step%s", pn, pn == 1 ? "" : "s");
			txt(s + b, fontSerif, 14, sf::Color(255, 150, 160), 18, ly);
		} else txt("search() path lights up when you pick something up", fontItalic, 13, TAG, 18, ly);
		ly += 28;
		float lx = 18;
		auto key2 = [&](sf::Color c, const std::string& s) {
			sf::CircleShape d(5.f); d.setOrigin({ 5.f, 5.f }); d.setPosition({ lx + 5, ly + 8 }); d.setFillColor(c);
			d.setOutlineColor(EDGE); d.setOutlineThickness(1); w->draw(d);
			lx += 14 + txt(s, fontSerif, 13, BONE, lx + 14, ly) + 22;
		};
		std::snprintf(b, sizeof b, "found %d/%d", found, n); key2(OPEN, b);
		std::snprintf(b, sizeof b, "clues read %d", read); key2(GOLD, b);
		key2(CHIP, "not yet");
		std::snprintf(b, sizeof b, "tree height %d", itemDB.height()); txt(b, fontMono, 10, TAG, W - 18 - width(b, fontMono, 10), ly + 4);
	}

	// ---- BFS + DFS: searching the house
	{
		float t = card(70, "Finding the way", "BFS  /  DFS");
		if (hintText.empty()) txt("route appears with the hint  (press H)", fontItalic, 13, TAG, 22, t);
		else if (lastRouteLen <= 1) { std::snprintf(b, sizeof b, "goal is in this room  -  %d steps of blood", trailLen); txt(b, fontSerif, 13, BONE, 22, t); }
		else {
			float x = 22;
			for (int i = 0; i < lastRouteLen; ++i) {
				std::string nm = ROOMS[lastRoute[i]].name;
				float cw = width(nm, fontSerif, 14) + 14;
				if (x + cw > W - 30) { txt("...", fontMono, 12, TAG, x, t + 4); break; }
				x += chip(x, t - 2, nm, i == lastRouteLen - 1 ? HI : CHIP, BONE);
				if (i + 1 < lastRouteLen) { arrow(x + 4, t + 9, TAG); x += 18; }
			}
		}
		if (lastDfsLen == 0) txt("DFS: hidden rooms not searched yet (read the Hymn Page)", fontItalic, 12, TAG, 22, t + 24);
		else { std::snprintf(b, sizeof b, "DFS: searched %d rooms through secret doors", lastDfsLen); txt(b, fontItalic, 12, TAG, 22, t + 24); }
	}
	// Scroll bar (mouse wheel or Page Up / Page Down over the panel).
	g_consoleContentH = y;
	w->setView(fixedV);
	if (g_consoleContentH > PH) {
		float trackH = PH - 16, barH = std::max(40.f, trackH * PH / g_consoleContentH);
		float barY = 8 + (trackH - barH) * g_consoleScroll / (g_consoleContentH - PH);
		rect(pw - 8, 8, 4, trackH, sf::Color(40, 12, 18));
		rect(pw - 8, barY, 4, barH, sf::Color(170, 40, 60));
	}
	w->setView(uiView(*w));
}

