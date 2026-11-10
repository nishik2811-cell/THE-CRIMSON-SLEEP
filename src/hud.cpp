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
	// BFS hint trail: small blood drops from the player to the objective.
	for (int i = 1; i < trailLen; i += 1) {
		int c = trail[i] % GRID_W, r = trail[i] / GRID_W;
		float pulse = 0.5f + 0.5f * std::sin(t * 4 - i * 0.5f);
		sf::CircleShape d(3.f + pulse);
		d.setOrigin({ (float)(d.getRadius()), (float)(d.getRadius()) });
		d.setPosition({ (float)(c * 32 + 16.f), (float)(r * 32 + 16.f) });
		d.setFillColor(sf::Color(200, 20, 40, (std::uint8_t)(110 + 120 * pulse)));
		w->draw(d);
	}
	// After two minutes the objective itself starts to pulse.
	int tc, tr;
	if (objectiveTime >= 120 && room == targetRoom && targetTile >= 0 && current_map->find(targetTile, &tc, &tr)) {
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
		sf::Text tx(fontTitle, wallText, 34);
		float a = wallT > 6 ? (7 - wallT) : wallT / 6;
		tx.setFillColor(sf::Color(150, 0, 16, (std::uint8_t)(230 * a)));
		tx.setPosition(wallPos);
		w->draw(tx);
		for (int i = 0; i < 6; ++i) {
			sf::RectangleShape drip(sf::Vector2f(2, 6 + (7 - wallT) * (4 + i % 3) * 3));
			drip.setPosition({ (float)(wallPos.x + 10 + i * 18), (float)(wallPos.y + 34) });
			drip.setFillColor(sf::Color(120, 0, 12, (std::uint8_t)(200 * a)));
			w->draw(drip);
		}
	}
}

void Game::drawHud(sf::RenderWindow* w) {
	w->setView(w->getDefaultView());
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
	std::snprintf(buf, sizeof buf, flareCooldown > 0 ? "Flare  %.0fs" : "Flare  ready (F)", flareCooldown);
	w->draw(label(fontSerif, player.has(LANTERN) ? buf : "No light", 17, flareCooldown > 0 ? ASH : BONE, 22, 72));

	// "[E] Talk" over Alfred's head when you're next to him.
	if (room == ENTRANCE_HALL && npc.state == Npc::standing && npc.near(&player)) {
		sf::Vector2i px = w->mapCoordsToPixel(npc.characterImage.getPosition() + sf::Vector2f(12.f, -30.f), player.view);
		sf::Vector2f sp = w->mapPixelToCoords(px, w->getDefaultView());
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

	// Top threat from the Priority Queue.
	if (topThreat.kind != TH_CALM && topThreat.label[0]) {
		sf::Text th(fontSerif, topThreat.label, 22);
		float pulse = topThreat.priority >= 75 ? 0.6f + 0.4f * std::sin(t * 10) : 1.f;
		th.setFillColor(sf::Color(230, 40, 60, (std::uint8_t)(255 * pulse)));
		th.setPosition({ (float)(std::floor((W - th.getLocalBounds().size.x) / 2)), (float)(120) });
		w->draw(th);
	}
	if (eventCaptionT > 0) {
		sf::Text ec(fontItalic, eventCaption, 22);
		ec.setFillColor(sf::Color(200, 170, 165, (std::uint8_t)(255 * (eventCaptionT > 1 ? 1 : eventCaptionT))));
		ec.setPosition({ (float)(std::floor((W - ec.getLocalBounds().size.x) / 2)), (float)(150) });
		w->draw(ec);
	}
	if (toastT > 0) {
		sf::Text tt(fontSerif, toast, 22);
		tt.setFillColor(sf::Color(240, 200, 120, (std::uint8_t)(255 * (toastT > 1 ? 1 : toastT))));
		tt.setPosition({ (float)(std::floor((W - tt.getLocalBounds().size.x) / 2)), (float)(86) });
		w->draw(tt);
	}

	// Hint (after 60 s) or the plain objective.
	if (!hintText.empty()) {
		panel(w, 12, H - 92, W - 24, 34, 200);
		w->draw(label(fontSerif, hintText, 19, sf::Color(255, 120, 130), 22, H - 89));
	}

	// Controls strip.
	w->draw(label(fontSerif, "E interact   F flare   B go back   H hint   I items   J tips   TAB data structures   ESC pause", 16, ASH, 22, H - 26));

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
		w->draw(label(fontItalic, "Space", 16, ASH, x + bw - 52, y + bh - 22));
	}

	drawMinimap(w);
	if (showInventory) drawInventory(w);
	if (showDebug) drawDebug(w);

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
			sf::RectangleShape f(sf::Vector2f(W, H)); f.setFillColor(sf::Color(200, 0, 20, 110)); w->draw(f);
		}
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
	const char* names[] = { "flicker", "whisper", "door slam", "breath", "blood writing", "apparition", "bells", "crack" };
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
	static const sf::Vector2i POS[ROOM_COUNT] = {
		{ -1, -1 }, { 2, 2 }, { 2, 1 }, { 1, 1 }, { 0, 1 }, { 1, 0 }, { 3, 1 }, { 2, 0 }, { 3, 0 }, { 4, 1 }, { 4, 0 } };
	static const char* SHORT[ROOM_COUNT] = { "", "Hall", "Corridor", "Library", "Bedroom", "Nursery", "Chapel", "Antechamber", "Crypt", "Tunnel", "Ossuary" };
	const float ox = 312, oy = 10, cw = 82, ch = 30, bw = 70, bh = 20;
	panel(w, ox - 8, oy - 4, 5 * cw + 4, 3 * ch + 8, 190);
	auto centre = [&](int r) { return sf::Vector2f(ox + POS[r].x * cw + bw / 2, oy + POS[r].y * ch + bh / 2 + 2); };
	auto known = [&](int r) {
		if (r == HIDDEN_TUNNEL || r == OSSUARY) { const Edge* e = house.edgeTo(CHAPEL, HIDDEN_TUNNEL); return e && !e->hidden; }
		return true;
	};
	bool onRoute[MAX_ROOMS] = {};
	if (!hintText.empty()) for (int i = 0; i < lastRouteLen; ++i) onRoute[lastRoute[i]] = true;
	// edges
	for (int r = 1; r < ROOM_COUNT; ++r) {
		if (!known(r)) continue;
		for (const Edge* e = house.neighbours(r); e; e = e->next) {
			if (e->to == OUTSIDE || e->to < r || e->hidden || !known(e->to)) continue;
			sf::Color col = e->locked ? sf::Color(110, 10, 24) : sf::Color(150, 120, 112);
			if (onRoute[r] && onRoute[e->to]) col = sf::Color(255, 120, 130);
			sf::Vertex ln[2] = { sf::Vertex{centre(r), col}, sf::Vertex{centre(e->to), col} };
			w->draw(ln, 2, sf::PrimitiveType::Lines);
		}
	}
	// nodes
	float t = runStats.time;
	for (int r = 1; r < ROOM_COUNT; ++r) {
		if (!known(r)) continue;
		sf::RectangleShape box(sf::Vector2f(bw, bh));
		box.setPosition({ (float)(ox + POS[r].x * cw), (float)(oy + POS[r].y * ch + 2) });
		bool here = r == room;
		float pulse = 0.6f + 0.4f * std::sin(t * 5);
		box.setFillColor(here ? sf::Color((std::uint8_t)(150 * pulse), 10, 30) : roomVisited[r] ? sf::Color(40, 10, 16) : sf::Color(16, 6, 8));
		box.setOutlineColor(onRoute[r] ? sf::Color(255, 120, 130) : r == targetRoom && objectiveTime >= 60 ? sf::Color(255, 200, 120) : sf::Color(90, 20, 30));
		box.setOutlineThickness(1);
		w->draw(box);
		sf::Text tx(fontMono, roomVisited[r] ? SHORT[r] : "?", 10);
		tx.setFillColor(here ? sf::Color::White : roomVisited[r] ? BONE : ASH);
		sf::FloatRect b = tx.getLocalBounds();
		tx.setPosition({ (float)(std::floor(box.getPosition().x + (bw - b.size.x) / 2 - b.position.x)), (float)(std::floor(box.getPosition().y + 4)) });
		w->draw(tx);
	}
}
