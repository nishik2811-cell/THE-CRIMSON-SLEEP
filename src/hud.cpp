// Graphics & UI handler: translates the data structures into pixels.
#include "game.h"
#include "ds/MinMax.h"
#include <cmath>
#include <cstdio>

namespace {
const sf::Color CRIMSON(176, 30, 58), BLOOD(120, 6, 18), BONE(232, 220, 200), ASH(150, 140, 136), INK(8, 2, 4);

sf::Text label(const sf::Font& f, const Str& s, unsigned size, sf::Color c, float x, float y) {
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
		// The drops are drawn over the darkness - and so over the hero too.  A drop
		// the hero is standing on is under his feet: skip it (his body hides it).
		sf::FloatRect body = player.characterImage.getGlobalBounds();
		for (int i = here - 1; i >= 1 && shown < 4; --i) {
			if (i % 3 != 0) continue;
			int c = trail[i] % GRID_W, r = trail[i] / GRID_W;
			int h = (c * 73856093) ^ (r * 19349663);
			float jx = (float)(((h >> 6) & 7) - 3), jy = (float)(((h >> 9) & 7) - 3);
			if (body.findIntersection(sf::FloatRect({ c * 32 + 8.f + jx, r * 32 + 8.f + jy }, { 16.f, 16.f }))) { ++shown; continue; }
			decalSprite.setTextureRect(sf::IntRect({ 16, 0 }, { 16, 16 }));   // the drip
			decalSprite.setPosition({ c * 32 + 8.f + jx, r * 32 + 8.f + jy });
			decalSprite.setColor(sf::Color(255, 255, 255, (std::uint8_t)(255 * (1.f - 0.15f * shown))));
			w->draw(decalSprite);
			++shown;
		}
	}
	// Words written in blood on the wall (they drip as they fade).
	if (wallT > 0) {
		// Scrawled onto the wall face letter by letter: each letter tilts,
		// bobs and changes size a little (seeded by its place, so it holds
		// still), with a darker smear underneath - written by a hand, not typed.
		float a = wallT > 6 ? (7 - wallT) : wallT / 6;
		float x = std::floor(wallPos.x);
		unsigned seed = (unsigned)(wallPos.x * 13 + wallPos.y * 7);
		for (int k = 0; k < wallText.size(); ++k) {
			unsigned h = seed * 2654435761u + (unsigned)k * 40503u;
			float tilt = (float)((int)(h % 25) - 12);                    // -12 .. 12 degrees
			float bob = (float)((int)((h >> 5) % 5) - 2);                 // -2 .. 2 px
			unsigned size = WALL_TEXT_SIZE + (unsigned)((h >> 9) % 4) - 1;  // a bit uneven
			char one[2] = { wallText[k], 0 };
			sf::Text letter(fontBlood, Str(one), size);
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

	// Hiding in a wardrobe: the view stays exactly as it is - just a small label.
	if (hidden) {
		sf::Text ht(fontSerif, "HIDDEN  -  E to step out", 17);
		ht.setLetterSpacing(1.3f);
		float tw = ht.getLocalBounds().size.x;
		panel(w, std::floor((W - tw) / 2) - 16, H - 138, tw + 32, 30, 200);
		ht.setFillColor(sf::Color(220, 205, 195)); ht.setPosition({ std::floor((W - tw) / 2), H - 134.f }); w->draw(ht);
	}

	// Status panel.
	panel(w, 12, 12, 270, 122);
	bar(w, fontSerif, "Health", 22, 18, player.health, sf::Color(170, 16, 30));
	bar(w, fontSerif, "Fear", 22, 44, player.fear, sf::Color(110, 60, 160));
	char buf[64];
	// Flare recharge: a bar that refills, with the seconds left on it.
	if (!player.has(LANTERN)) w->draw(label(fontSerif, "No light", 17, ASH, 22, 72));
	else {
		float ready = 1.f - flareCooldown / FLARE_COOLDOWN;
		ready = maxOf(0.f, minOf(1.f, ready));
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
	// Stomp recharge: the same kind of bar, under the flare.
	{
		float ready = maxOf(0.f, minOf(1.f, 1.f - stompT / STOMP_COOLDOWN));
		bool full = stompT <= 0;
		float glow = full ? 0.75f + 0.25f * std::sin(t * 4 + 1.5f) : 1.f;
		sf::Color fillCol = full ? sf::Color((std::uint8_t)(235 * glow), (std::uint8_t)(222 * glow), (std::uint8_t)(200 * glow)) : sf::Color(110, 95, 85);
		bar(w, fontSerif, "Stomp", 22, 96, ready * 100.f, fillCol);
		if (full) std::snprintf(buf, sizeof buf, "ready - press Q");
		else std::snprintf(buf, sizeof buf, "%.0fs", std::ceil(stompT));
		sf::Text st(fontMono, buf, 11);
		st.setFillColor(full ? sf::Color(30, 12, 8) : BONE);
		sf::FloatRect lb = st.getLocalBounds();
		st.setOrigin({ lb.position.x + lb.size.x / 2.f, lb.position.y + lb.size.y / 2.f });
		st.setPosition({ 92.f + 90.f, 102.f + 6.f });
		w->draw(st);
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

	// The Devil's health: a wide boss bar, bottom-centre above the hint line.
	if (room == CRIMSON_CRYPT && bossActive) {   // gone the moment he breaks
		const float bw = 500, bx = (W - bw) / 2, by = H - 108;
		sf::Text name(fontTitle, "The Devil", 24); name.setFillColor(CRIMSON);
		name.setOutlineColor(sf::Color::Black); name.setOutlineThickness(2);
		name.setPosition({ std::floor((W - name.getLocalBounds().size.x) / 2), by - 32 }); w->draw(name);
		sf::RectangleShape back({ bw, 12.f }); back.setPosition({ bx, by });
		back.setFillColor(sf::Color(24, 6, 10, 230)); back.setOutlineColor(sf::Color(120, 20, 34)); back.setOutlineThickness(2); w->draw(back);
		float frac = maxOf(0.f, (float)bossHP / BOSS_MAX_HP);
		sf::RectangleShape fill({ (bw - 4) * frac, 8.f }); fill.setPosition({ bx + 2, by + 2 });
		fill.setFillColor(bossHitT > 0 ? sf::Color(255, 140, 140) : sf::Color(190, 18, 38)); w->draw(fill);
		for (int i = 1; i < BOSS_MAX_HP; ++i) {         // notches: one per blow
			sf::RectangleShape n({ 2.f, 12.f }); n.setPosition({ bx + bw * i / BOSS_MAX_HP - 1, by });
			n.setFillColor(sf::Color(10, 2, 4, 200)); w->draw(n);
		}
	}


	// Door transition: fade to black as the door opens, back in as it shuts behind you.
	{
		float fade = 0;
		if (doorT >= DOOR_OPEN) fade = minOf(1.f, (doorT - DOOR_OPEN) / DOOR_FADE);
		if (arriveT >= 0) fade = maxOf(0.f, 1.f - arriveT / 0.3f);
		if (fade > 0) { sf::RectangleShape black({ 1024.f, 768.f }); black.setFillColor(sf::Color(0, 0, 0, (std::uint8_t)(255 * fade))); w->draw(black); }
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
	w->draw(label(fontSerif, player.has(SWORD) ? "E interact   SHIFT sword   C block   F flare   Q stomp   L lantern   B back   H hint   I items   J tips   TAB   M   ESC" : "E interact   F flare   Q stomp   L lantern   B back   H hint   I items   J tips   TAB console   M mute   ESC pause", 15, ASH, 22, H - 26));

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

	drawAlfredScare(w);                                  // dead Alfred's face, if he just got you
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
	drawTipCard(w);
	if (tipOpen >= 0) drawTip(w, tipOpen);
	if (journalPage >= 0) drawJournal(w);
}

// Word-wraps `text` (keeping its own line breaks) to fit `maxW` pixels.
static Str wrapText(const sf::Font& f, unsigned size, const Str& text, float maxW) {
	Str out, line, word;
	auto width = [&](const Str& s) { sf::Text t(f, s, size); return t.getLocalBounds().size.x; };
	auto flushWord = [&]() {
		if (word.empty()) return;
		Str trial = line.empty() ? word : line + " " + word;
		if (!line.empty() && width(trial) > maxW) { out += line; out += '\n'; line = Str("  ") + word; }   // indent the run-on
		else line = trial;
		word.clear();
	};
	for (int i = 0; i <= text.size(); ++i) {
		char ch = i < text.size() ? text[i] : '\n';
		if (ch == '\n' && i + 1 < text.size() && text[i + 1] == ' ') ch = ' ';   // an indented next line continues this one
		if (ch == ' ') flushWord();
		else if (ch == '\n') { flushWord(); out += line; if (i < text.size()) out += '\n'; line.clear(); }
		else word += ch;
	}
	return out;
}

// A survival note as an ornate card at the left, under the status panel: dark
// crimson glass, a double border with corner diamonds, blood dripping from the
// top edge.  It slides in, stays ~10 s while you keep playing, and fades.
void Game::drawTipCard(sf::RenderWindow* w) {
	if (tipCard < 0 || tipCardT <= 0) return;
	float age = TIP_CARD_TIME - tipCardT;
	float in = minOf(1.f, age / 0.35f), out = minOf(1.f, tipCardT / 0.8f);
	float a = minOf(in, out);
	auto A = [&](float v) { return (std::uint8_t)maxOf(0.f, minOf(255.f, v * a)); };
	const float wd = 350, x = 12 - (1 - in) * (wd + 20);
	Str body = wrapText(fontSerif, 15, TIPS[tipCard].body, wd - 34);
	sf::Text bodyT(fontSerif, body, 15);
	float h = 70 + bodyT.getLocalBounds().size.y + 26;
	const float y = std::floor((768.f - h) / 2);          // at the left side, centred vertically

	// glass: a translucent gradient, darker at the bottom
	sf::Vertex g[4] = {
		{ { x, y }, sf::Color(26, 4, 9, A(200)) }, { { x + wd, y }, sf::Color(26, 4, 9, A(200)) },
		{ { x, y + h }, sf::Color(6, 1, 3, A(225)) }, { { x + wd, y + h }, sf::Color(6, 1, 3, A(225)) } };
	w->draw(g, 4, sf::PrimitiveType::TriangleStrip);
	// double border
	sf::RectangleShape outer({ wd, h }); outer.setPosition({ x, y }); outer.setFillColor(sf::Color::Transparent);
	outer.setOutlineColor(sf::Color(170, 22, 40, A(230))); outer.setOutlineThickness(1.5f); w->draw(outer);
	sf::RectangleShape inner({ wd - 10, h - 10 }); inner.setPosition({ x + 5, y + 5 }); inner.setFillColor(sf::Color::Transparent);
	inner.setOutlineColor(sf::Color(120, 70, 50, A(120))); inner.setOutlineThickness(1.f); w->draw(inner);
	// corner diamonds
	auto diamond = [&](float cx, float cy, float r, sf::Color c) {
		sf::Vertex d[4] = { { { cx, cy - r }, c }, { { cx + r, cy }, c }, { { cx - r, cy }, c }, { { cx, cy + r }, c } };
		w->draw(d, 4, sf::PrimitiveType::TriangleStrip);
	};
	sf::Color gem(200, 30, 50, A(255));
	diamond(x, y, 5, gem); diamond(x + wd, y, 5, gem); diamond(x, y + h, 5, gem); diamond(x + wd, y + h, 5, gem);
	// Blood running down the card (the team's drip photo, cut out): drawn in thin
	// strips, each revealed downwards at its own pace - the pooled band first,
	// then the runs creep down and the long ones keep stretching.  A bead swells
	// at the end of each run and now and then lets go.
	if (bloodTex.getSize().x > 0) {
		const float sx = wd / bloodTex.getSize().x, sy = minOf(1.f, (h * 0.78f) / bloodTex.getSize().y);
		const int strips = (int)(bloodTex.getSize().x / 3);
		for (int s = 0; s < strips && s < 128; ++s) {
			float seed = std::fabs(std::sin(s * 12.9898f) * 43758.5453f); seed -= std::floor(seed);   // 0..1 per strip
			float speed = 26.f + 40.f * seed;                                       // px per second, before scaling
			float shown = minOf((float)bloodTip[s] + 2, 10.f + speed * age * (0.6f + 0.4f * minOf(1.f, age)));
			if (shown <= 0) continue;
			bloodSprite.setTextureRect(sf::IntRect({ s * 3, 0 }, { 3, (int)shown }));
			bloodSprite.setScale({ sx, sy });
			bloodSprite.setPosition({ x + s * 3 * sx, y });
			bloodSprite.setColor(sf::Color(175, 160, 160, A(140)));   // dulled: it sits behind the text
			w->draw(bloodSprite);
			// the run has reached its full length: a bead hangs there and sometimes drops
			if (shown >= bloodTip[s] && bloodTip[s] > 40 && seed > 0.55f) {
				float cyc = std::fmod(age * (0.5f + seed) + seed * 7.f, 3.f);
				float tipY = y + bloodTip[s] * sy, cx = x + (s * 3 + 1.5f) * sx;
				float r = 1.2f + 1.3f * minOf(1.f, cyc / 1.8f);
				if (cyc < 2.2f) {
					sf::CircleShape bead(r); bead.setOrigin({ r, r }); bead.setPosition({ cx, tipY + r * 0.6f });
					bead.setFillColor(sf::Color(100, 8, 16, A(170))); w->draw(bead);
				}
				else {
					float t = cyc - 2.2f, fy = tipY + t * t * 160.f;
					if (fy < y + h - 10) {
						sf::CircleShape drop(r); drop.setOrigin({ r, r }); drop.setScale({ 0.75f, 1.3f });
						drop.setPosition({ cx, fy }); drop.setFillColor(sf::Color(120, 8, 18, A(220 * (1 - t / 0.8f)))); w->draw(drop);
					}
				}
			}
		}
	}
	// header: "survival note", the title, an ornamental rule
	sf::Text tag(fontItalic, "survival note", 14); tag.setFillColor(sf::Color(220, 190, 180, A(235)));
	tag.setOutlineColor(sf::Color(0, 0, 0, A(200))); tag.setOutlineThickness(1.f);
	tag.setPosition({ x + 16, y + 9 }); w->draw(tag);
	sf::Text more(fontItalic, "J - all notes", 13); more.setFillColor(sf::Color(210, 170, 165, A(220)));
	more.setOutlineColor(sf::Color(0, 0, 0, A(200))); more.setOutlineThickness(1.f);
	more.setPosition({ x + wd - 16 - more.getLocalBounds().size.x, y + 10 }); w->draw(more);
	sf::Text title(fontTitle, TIPS[tipCard].title, 24);
	while (title.getLocalBounds().size.x > wd - 32 && title.getCharacterSize() > 14) title.setCharacterSize(title.getCharacterSize() - 1);
	title.setFillColor(sf::Color(CRIMSON.r, CRIMSON.g, CRIMSON.b, A(255)));
	title.setOutlineColor(sf::Color(0, 0, 0, A(200))); title.setOutlineThickness(1.5f);
	title.setPosition({ std::floor(x + (wd - title.getLocalBounds().size.x) / 2), y + 26 }); w->draw(title);
	float ry = y + 60;
	sf::Vertex rule[2] = { { { x + 30, ry }, sf::Color(170, 22, 40, A(0)) }, { { x + wd / 2, ry }, sf::Color(170, 22, 40, A(220)) } };
	sf::Vertex rule2[2] = { { { x + wd / 2, ry }, sf::Color(170, 22, 40, A(220)) }, { { x + wd - 30, ry }, sf::Color(170, 22, 40, A(0)) } };
	w->draw(rule, 2, sf::PrimitiveType::Lines); w->draw(rule2, 2, sf::PrimitiveType::Lines);
	diamond(x + wd / 2, ry, 3.5f, gem);
	// the rules themselves, wrapped to fit
	bodyT.setFillColor(sf::Color(BONE.r, BONE.g, BONE.b, A(250)));
	bodyT.setOutlineColor(sf::Color(0, 0, 0, A(220))); bodyT.setOutlineThickness(1.2f);
	bodyT.setPosition({ x + 17, y + 68 }); w->draw(bodyT);
	// time left: a thin glowing line along the bottom
	float tl = (wd - 24) * tipCardT / TIP_CARD_TIME;
	sf::RectangleShape glow({ tl, 3.f }); glow.setPosition({ x + 12, y + h - 9 }); glow.setFillColor(sf::Color(200, 30, 50, A(90))); w->draw(glow);
	sf::RectangleShape line({ tl, 1.f }); line.setPosition({ x + 12, y + h - 8 }); line.setFillColor(sf::Color(255, 90, 110, A(230))); w->draw(line);
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
		w->draw(label(fontSerif, Str("- ") + ITEMS[it.id].name, 19, BONE, 28, y));
		y += 26;
	});
}

void Game::drawDebug(sf::RenderWindow* w) {
	panel(w, 520, 56, 492, 610, 235);
	float x = 532, y = 62;
	auto line = [&](const Str& s, sf::Color c = BONE, unsigned size = 13) { w->draw(label(fontMono, s, size, c, x, y)); y += size + 5; };
	auto head = [&](const Str& s) { y += 4; line(s, sf::Color(255, 90, 110), 14); };
	char b[160];

	head("GRAPH  adjacency list of " + Str(ROOMS[room].name));
	for (const Edge* e = house.neighbours(room); e; e = e->next) {
		std::snprintf(b, sizeof b, "  door %d -> %-20s %s%s", e->doorNo, ROOMS[e->to].name, e->hidden ? "[hidden] " : "", e->locked ? (Str("[locked: ") + ITEMS[e->requiredItem].name + "]").c_str() : "[open]");
		line(b);
	}
	head("STACK  movement history (top first)");
	int hist[32]; int hn = player.history.toArray(hist, 32);
	Str s = "  ";
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
		line(Str(b).substr(0, 62), i == 0 ? sf::Color(255, 160, 160) : BONE);
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
	for (int i = 0; i < lastDfsLen; ++i) { s += Str::num(lastDfs[i]); if (i + 1 < lastDfsLen) s += " > "; }
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

// The live data-structure console (its own strip, opened with TAB or the
// console button).  A quiet, technical layout - not the horror theme: one
// neutral palette, one accent, hairline-separated sections on a strict grid
// (24 px margin, 8/16/24 spacing).  Each section DRAWS its structure with
// the live game data, and sections that hold more than fits open on a click.
void Game::drawConsole(sf::RenderWindow* w) {
	// The strip beside the game.  Its view is always CONSOLE_PANEL_W wide and as
	// tall as the strip's real shape allows, so text is never stretched; a
	// taller window simply shows more of the console at once.
	const float pw = CONSOLE_PANEL_W;
	const float stripPx = maxOf(1.f, g_panelVP.size.x * (float)w->getSize().x);
	const float PH = pw * (float)w->getSize().y / stripPx;
	sf::View fixedV(sf::FloatRect({ 0.f, 0.f }, { pw, PH }));
	fixedV.setViewport(g_panelVP);
	sf::View scrollV = fixedV;
	if (g_consoleContentH > 0) g_consoleScroll = clampTo(g_consoleScroll, 0.f, maxOf(0.f, g_consoleContentH - PH));   // (height is known after the first frame)
	scrollV.move({ 0.f, std::floor(g_consoleScroll) });

	// ---- palette: neutrals, one accent, two status colours
	const sf::Color BG(19, 19, 21), RULE(42, 42, 46), BOX(27, 27, 30), BOX_EDGE(56, 56, 61), LINK(92, 92, 98);
	const sf::Color TEXT(228, 227, 223), BODY(196, 195, 191), MUTED(138, 138, 136), FAINT(92, 92, 94);
	const sf::Color ACCENT(226, 104, 86), ACCENT_BG(44, 28, 27);
	const sf::Color GOOD(132, 196, 140), BAD(222, 108, 96), WARM(214, 172, 96);

	w->setView(fixedV);
	sf::RectangleShape bg({ pw, PH }); bg.setFillColor(BG); w->draw(bg);
	w->setView(scrollV);
	const float M = 24, R = pw - 24, CW = R - M;        // margins and content width
	const float NODE_H = 26, ROW = 34, GAP = 24;        // node height, wrapped-row pitch, arrow gap

	// ---- drawing helpers
	auto make = [&](const Str& s, unsigned size, bool bold, float spacing) {
		sf::Text t(fontMono, sf::String::fromUtf8(s.begin(), s.end()), size);
		if (bold) t.setStyle(sf::Text::Bold);
		if (spacing != 1.f) t.setLetterSpacing(spacing);
		return t;
	};
	// align: 0 left, 1 centre, 2 right.  Returns the drawn width.
	auto text = [&](const Str& s, unsigned size, sf::Color c, float x, float y, bool bold = false, int align = 0, float spacing = 1.f) {
		sf::Text t = make(s, size, bold, spacing);
		t.setFillColor(c);
		sf::FloatRect b = t.getLocalBounds();
		float px = align == 0 ? x : align == 1 ? x - b.size.x / 2 - b.position.x : x - b.size.x - b.position.x;
		t.setPosition({ std::floor(px), std::floor(y) });
		w->draw(t);
		return b.size.x;
	};
	auto tw = [&](const Str& s, unsigned size, bool bold = false, float spacing = 1.f) { return make(s, size, bold, spacing).getLocalBounds().size.x; };
	auto label = [&](const Str& s, sf::Color c, float x, float y, int align = 0) { return text(s, 10, c, x, y, true, align, 1.6f); };   // small caps-style label
	auto rrect = [&](float x, float y, float wd, float h, float r, sf::Color fill, sf::Color line = sf::Color::Transparent, float thick = 1.f) {
		r = minOf(r, minOf(wd, h) / 2);
		sf::ConvexShape s(24);
		for (int c = 0; c < 4; ++c) {                       // 6 points per rounded corner
			float cx = c == 0 || c == 3 ? x + r : x + wd - r, cy = c < 2 ? y + r : y + h - r;
			float a0 = 3.14159f + c * 1.5708f;
			for (int k = 0; k < 6; ++k) {
				float a = a0 + k * (1.5708f / 5);
				s.setPoint(c * 6 + k, { cx + std::cos(a) * r, cy + std::sin(a) * r });
			}
		}
		s.setFillColor(fill);
		if (line.a) { s.setOutlineColor(line); s.setOutlineThickness(-thick); }
		w->draw(s);
	};
	auto hline = [&](float x0, float x1, float yy, sf::Color c) { sf::RectangleShape r({ x1 - x0, 1.f }); r.setPosition({ x0, std::floor(yy) }); r.setFillColor(c); w->draw(r); };
	auto seg = [&](sf::Vector2f a, sf::Vector2f b, sf::Color c) { sf::Vertex v[2] = { { a, c }, { b, c } }; w->draw(v, 2, sf::PrimitiveType::Lines); };
	auto arrow = [&](float x, float yy, float len, sf::Color c) {   // right-pointing, solid head
		hline(x, x + len - 5, yy, c);
		sf::ConvexShape h(3); h.setPoint(0, { x + len, yy + 0.5f }); h.setPoint(1, { x + len - 5, yy - 3 }); h.setPoint(2, { x + len - 5, yy + 4 });
		h.setFillColor(c); w->draw(h);
	};
	auto nodeW = [&](const Str& s) { return tw(s, 12) + 22; };
	auto node = [&](float x, float yy, const Str& s, bool hi, float wd = 0) {
		if (wd <= 0) wd = nodeW(s);
		rrect(x, yy, wd, NODE_H, 3, hi ? ACCENT_BG : BOX, hi ? ACCENT : BOX_EDGE);
		text(s, 12, hi ? TEXT : BODY, x + 11, yy + 5);
		return wd;
	};

	// Clicks (from main) in panel coordinates: a section that can grow opens
	// to show every element; a second click folds it.
	bool clicked = g_consoleClick;
	sf::Vector2f clickAt = clicked ? w->mapPixelToCoords(g_consoleClickPx, scrollV) : sf::Vector2f();
	g_consoleClick = false;
	float y = 24;
	// A section: hairline, KIND label, title, one-line caption, and on the
	// right either a count or the show-all toggle.  Returns the body's top.
	auto section = [&](float bodyH, const Str& kind, const Str& title, const Str& caption, const Str& meta, int id = -1, bool more = false) {
		bool open = id >= 0 && cardOpen[id];
		float top = y;
		hline(M, R, y, RULE);
		y += 20;
		label(kind, ACCENT, M, y);
		if (id >= 0 && (more || open)) label(open ? "SHOW LESS" : "SHOW ALL", MUTED, R, y, 2);
		else if (!meta.empty()) text(meta, 11, FAINT, R, y - 1, false, 2);
		y += 18;
		text(title, 15, TEXT, M, y, true);
		y += 22;
		text(caption, 11, MUTED, M, y);
		y += 28;
		float body = y;
		y += bodyH + 24;
		if (id >= 0 && clicked && clickAt.x >= 0 && clickAt.x <= pw && clickAt.y >= top && clickAt.y < y) cardOpen[id] = !cardOpen[id];
		return body;
	};
	// Nodes joined by arrows, wrapping onto rows; draw=false only measures.  Returns rows used.
	auto flow = [&](const Str* names, int n, float top, bool draw, int hi, bool nullEnd) {
		float x = M; int rows = 1;
		for (int i = 0; i < n; ++i) {
			float wd = nodeW(names[i]);
			if (x + wd > R && x > M) { x = M; ++rows; }
			float ry = top + (rows - 1) * ROW;
			if (draw) node(x, ry, names[i], i == hi);
			x += wd;
			if (i + 1 < n || nullEnd) { if (draw) arrow(x + 5, ry + NODE_H / 2, GAP - 10, LINK); x += GAP; }
		}
		if (nullEnd) {
			float nw = tw("null", 11) + 4;
			if (x + nw > R) { x = M; ++rows; }
			if (draw) text("null", 11, FAINT, x, top + (rows - 1) * ROW + 6);
		}
		return rows;
	};
	char b[160];

	// ================= header: the room and its facts (ARRAY: ROOMS[]) =================
	std::snprintf(b, sizeof b, "ROOMS[%d]", (int)room);
	label(b, FAINT, M, y);
	label("TAB TO CLOSE", FAINT, R, y, 2);
	y += 20;
	text(ROOMS[room].name, 22, TEXT, M, y, true);
	y += 40;
	{
		// three facts in equal columns: label above, value below
		const float col = CW / 3;
		label("DANGER", FAINT, M, y);
		label("LIGHT", FAINT, M + col, y);
		label("DRIFTING GHOST", FAINT, M + 2 * col, y);
		float vy = y + 18;
		int d = ROOMS[room].danger;
		std::snprintf(b, sizeof b, "%d/10", d);
		float tx = text(b, 13, TEXT, M, vy, true);
		float bx = M + tx + 12, bw = col - tx - 36;
		for (int i = 0; i < 10; ++i) {                     // ten ticks
			float sw = (bw - 9 * 3) / 10;
			rrect(bx + i * (sw + 3), vy + 6, sw, 6, 1, i < d ? (d >= 7 ? BAD : d >= 4 ? WARM : GOOD) : BOX_EDGE);
		}
		text(ROOMS[room].dark ? "Dark" : "Lit", 13, TEXT, M + col, vy, true);
		text(ROOMS[room].wraith ? "Can appear" : "None", 13, ROOMS[room].wraith ? BAD : TEXT, M + 2 * col, vy, true);
		y += 54;
	}

	// ================= GRAPH: this room and its doors =================
	{
		static const char* SHORT[ROOM_COUNT] = { "", "Hall", "Corridor", "Library", "Bedroom", "Nursery", "Chapel", "Antechamber", "Crypt", "Tunnel", "Ossuary" };
		const Edge* list[6]; int n = 0;
		for (const Edge* e = house.neighbours(room); e && n < 6; e = e->next) if (!e->hidden) list[n++] = e;
		std::snprintf(b, sizeof b, "%d door%s", n, n == 1 ? "" : "s");
		float t = section(n ? 98.f : 26.f, "GRAPH", "Doors from here", "The house is an adjacency list: each room keeps a list of its doors.", b);
		float cx = M + CW / 2;
		if (n == 0) text("No doors you can see.", 12, FAINT, M, t + 4);
		else {
			float cw = tw(ROOMS[room].name, 12, true) + 28;
			rrect(cx - cw / 2, t, cw, NODE_H + 2, 3, ACCENT_BG, ACCENT);
			text(ROOMS[room].name, 12, TEXT, cx, t + 6, true, 1);
			float slot = CW / n, ny = t + 56;
			for (int i = 0; i < n; ++i) {
				const Edge* e = list[i];
				float nx = M + slot * i + slot / 2;
				Str name = e->to == OUTSIDE ? "Outside" : SHORT[e->to];
				seg({ cx, t + NODE_H + 2 }, { nx, ny }, LINK);
				float nw = minOf(slot - 8, nodeW(name));
				rrect(nx - nw / 2, ny, nw, NODE_H, 3, BOX, BOX_EDGE);
				text(name, 12, BODY, nx, ny + 5, false, 1);
				// The door's real state, by the same rule the game uses (edgeUsable):
				// open / locked but you can open it now / locked, and what it needs.
				bool can = edgeUsable(*e);
				Str st; sf::Color sc;
				if (!e->locked) { st = "open"; sc = GOOD; }
				else if (e->to == OUTSIDE) { st = can ? "opens now" : "sealed"; sc = can ? WARM : BAD; }
				else if (can) { st = e->requiredItem == NEWSPAPER ? "clue read: opens" : Str("you have the ") + ITEMS[e->requiredItem].name; sc = WARM; }
				else { st = e->requiredItem == NEWSPAPER ? "read the 1953 newspaper" : Str("needs ") + ITEMS[e->requiredItem].name; sc = BAD; }
				if (tw(st, 10) > slot - 6) st = !e->locked ? "open" : can ? "can open" : "locked";
				text(st, 10, sc, nx, ny + NODE_H + 6, false, 1);
			}
		}
	}

	// ================= STACK: rooms visited =================
	{
		int hist[32]; int hn = player.history.toArray(hist, 32);
		bool open = cardOpen[0];
		int show = open ? hn : minOf(hn, 3);
		std::snprintf(b, sizeof b, "%d item%s", hn, hn == 1 ? "" : "s");
		float t = section(show * ROW - 8 + (hn > show ? 24.f : 0.f), "STACK", "Where you've been",
		                  "Each room you enter is pushed on top. B pops it to go back.", b, 0, hn > 3);
		const float IX = M + 28, BW = 236;                    // index column, then the boxes
		for (int i = 0; i < show; ++i) {
			float by = t + i * ROW;
			bool top = i == 0;
			std::snprintf(b, sizeof b, "%d", hn - 1 - i);
			text(b, 11, FAINT, IX - 10, by + 6, false, 2);
			rrect(IX, by, BW, NODE_H, 3, top ? ACCENT_BG : BOX, top ? ACCENT : BOX_EDGE);
			text(ROOMS[hist[i]].name, 12, top ? TEXT : BODY, IX + 11, by + 5, top);
			float ax = IX + BW + 16;
			if (top) text("top \xE2\x80\x94 you are here", 11, ACCENT, ax, by + 6);
			else if (i == 1) text("B takes you back here", 11, MUTED, ax, by + 6);
			else if (i == hn - 1) text("bottom \xE2\x80\x94 where you woke", 11, FAINT, ax, by + 6);
		}
		if (hn > show) { std::snprintf(b, sizeof b, "%d more below", hn - show); text(b, 11, FAINT, IX + 11, t + show * ROW); }
	}

	// ================= QUEUE: scares, first in first out =================
	{
		const char* what[] = { "Flicker", "Whisper", "Door slam", "Breath", "Blood writing", "Moan", "Floor crack", "Glass" };   // EventKind order
		HauntEvent ev[16]; int en = events.toArray(ev, 16);
		Str names[16];
		for (int i = 0; i < en; ++i) { std::snprintf(b, sizeof b, "%s  %.0fs", what[ev[i].kind], maxOf(0.f, ev[i].at - roomTime)); names[i] = b; }
		int fit = 0; { float x = M; for (int i = 0; i < en; ++i) { float wd = nodeW(names[i]); if (x + wd > R - 56) break; x += wd + GAP; ++fit; } }
		bool open = cardOpen[1];
		int rows = open && en ? flow(names, en, 0, false, 0, false) : 1;
		std::snprintf(b, sizeof b, "%d queued", en);
		float t = section(rows * ROW - 8 + 16, "QUEUE", "Next scares in this room", "First in, first out: the front fires next, new ones join the back.", b, 1, fit < en);
		if (en == 0) text("Empty \xE2\x80\x94 the room is quiet for now.", 12, FAINT, M, t + 4);
		else {
			int drawn = open ? en : fit;
			flow(names, drawn, t, true, 0, false);
			label("FRONT", MUTED, M, t + NODE_H + 8);
			if (!open && fit < en) { std::snprintf(b, sizeof b, "+%d", en - fit); text(b, 12, MUTED, R, t + 5, false, 2); }
			else if (drawn > 1 && rows == 1) {
				float x = M; for (int i = 0; i < drawn - 1; ++i) x += nodeW(names[i]) + GAP;
				label("BACK", MUTED, x, t + NODE_H + 8);
			}
		}
	}

	// ================= PRIORITY QUEUE: dangers, worst first =================
	{
		PriorityQueue<Threat> order = threats;               // pop a copy: extractMax returns them worst first
		Threat list[16]; int tn = 0; Threat th;
		while (tn < 16 && order.extractMax(&th)) if (th.kind != TH_CALM && th.label[0]) list[tn++] = th;
		bool open = cardOpen[2];
		int show = open ? tn : minOf(tn, 3);
		const float PR = 36;
		std::snprintf(b, sizeof b, "%d active", tn);
		float t = section(maxOf(1, show) * PR - 12 + (tn > show ? 20.f : 0.f), "PRIORITY QUEUE", "Dangers right now",
		                  "A max-heap: whatever is most dangerous is always on top.", b, 2, tn > 3);
		if (!tn) text("Calm \xE2\x80\x94 nothing is hunting you.", 12, GOOD, M, t + 4);
		const float LX = M + 40;                              // label column
		for (int i = 0; i < show; ++i) {
			const Threat& q = list[i];
			float by = t + i * PR;
			bool top = i == 0;
			std::snprintf(b, sizeof b, "%d", q.priority);
			text(b, 13, top ? ACCENT : MUTED, LX - 12, by, true, 2);
			Str lbl = q.label; if (tw(lbl, 12) > R - LX) { while (lbl.size() > 4 && tw(lbl + "...", 12) > R - LX) lbl = lbl.substr(0, lbl.size() - 1); lbl += "..."; }
			text(lbl, 12, top ? TEXT : BODY, LX, by);
			rrect(LX, by + 19, R - LX, 3, 1.5f, BOX_EDGE);
			rrect(LX, by + 19, maxOf(3.f, (R - LX) * q.priority / 100.f), 3, 1.5f, top ? ACCENT : MUTED);
		}
		if (tn > show) { std::snprintf(b, sizeof b, "%d more", tn - show); text(b, 11, FAINT, LX, t + show * PR); }
	}

	// ================= LINKED LIST: inventory =================
	{
		Str names[16]; int n = 0;
		player.inventory.forEach([&](const InventoryItem& it) { if (n < 16) names[n++] = ITEMS[it.id].name; });
		int rows = n ? flow(names, n, 0, false, 0, true) : 1;
		bool open = cardOpen[3];
		std::snprintf(b, sizeof b, "%d item%s", n, n == 1 ? "" : "s");
		float t = section((open ? rows : 1) * ROW - 8 + 16, "LINKED LIST", "Carrying", "Each item points to the next; a new item goes in at the head.", b, 3, rows > 1);
		if (n == 0) text("head \xE2\x86\x92 null  (nothing yet)", 12, FAINT, M, t + 4);
		else if (open || rows == 1) flow(names, n, t, true, 0, true);
		else {
			float x = M; int k = 0;
			for (int i = 0; i < n; ++i) {
				float wd = nodeW(names[i]);
				if (x + wd + 64 > R) break;
				node(x, t, names[i], i == 0); x += wd;
				arrow(x + 5, t + NODE_H / 2, GAP - 10, LINK); x += GAP; ++k;
			}
			std::snprintf(b, sizeof b, "+%d more", n - k); text(b, 12, MUTED, x, t + 5);
		}
		if (n) label("HEAD", MUTED, M, t + NODE_H + 8);
	}

	// ================= ENEMY AI: the spiders' state machines =================
	{
		const char* names[] = { "IDLE", "PATROL", "INVEST", "SEARCH", "CHASE", "ATTACK", "STUN", "RETURN" };
		const float SR = 56;
		std::snprintf(b, sizeof b, "%d spider%s", spiderCount, spiderCount == 1 ? "" : "s");
		float t = section(spiderCount ? spiderCount * SR - 16 : 18.f, "STATE MACHINE  /  A*", "Enemy AI \xE2\x80\x94 the Weavers",
		                  "Each spider is in one of 8 states; it paths to you with A* or BFS.", b);
		if (!spiderCount) text("No spiders in this room.", 12, FAINT, M, t + 2);
		const float SX = M + 76, segW = (R - SX) / 8.f;
		for (int i = 0; i < spiderCount; ++i) {
			const Spider& s = spiders[i];
			float y0 = t + i * SR;
			std::snprintf(b, sizeof b, "Weaver %d", i + 1);
			text(b, 12, s.active ? TEXT : FAINT, M, y0 + 4, true);
			rrect(SX, y0, R - SX, 22, 3, BOX, BOX_EDGE);         // one segmented control
			for (int k = 0; k < 8; ++k) {
				bool on = s.active && (int)s.state == k;
				if (on) rrect(SX + k * segW + 1, y0 + 1, segW - 2, 20, 2, ACCENT);
				else if (k) { sf::RectangleShape dv({ 1.f, 12.f }); dv.setPosition({ SX + k * segW, y0 + 5 }); dv.setFillColor(RULE); w->draw(dv); }
				text(names[k], 9, on ? sf::Color(26, 16, 14) : FAINT, SX + k * segW + segW / 2, y0 + 5, on, 1);
			}
			const char* sense = !s.active ? "squashed" : s.state == Spider::STUNNED ? "stunned by the flare" :
			                    s.sawYou ? "sees you" : s.heardYou ? "heard you" : hidden ? "you are hidden" : "senses nothing";
			sf::Color sc = !s.active ? FAINT : s.sawYou ? BAD : s.heardYou ? WARM : MUTED;
			text(sense, 11, sc, SX, y0 + 28);
			if (s.active && s.pathLen > 0 && s.algo[0]) {
				int left = s.pathLen - s.pathStep;
				std::snprintf(b, sizeof b, "%s  \xC2\xB7  %d tile%s to go", s.algo, left, left == 1 ? "" : "s");
				text(b, 11, MUTED, R, y0 + 28, false, 2);
			}
		}
	}

	// ================= BST: items and clues =================
	{
		const int MAXN = 32;
		int key[MAXN], depth[MAXN], parent[MAXN]; ItemRecord rec[MAXN]; int n = 0;
		itemDB.walk([&](int k, const ItemRecord& r, int d, int p) { if (n < MAXN) { key[n] = k; rec[n] = r; depth[n] = d; parent[n] = p; ++n; } });
		const float RAD = 12.f, ystep = 32.f;
		float treeH = (itemDB.height() - 1) * ystep + 2 * RAD;
		std::snprintf(b, sizeof b, "height %d", itemDB.height());
		float t = section(treeH + 58, "BINARY SEARCH TREE", "Items and clues", "Every item, keyed by its id. Smaller ids go left, larger go right.", b);
		float step = CW / maxOf(1, n);
		auto at = [&](int i) { return sf::Vector2f(M + step * (i + 0.5f), t + RAD + depth[i] * ystep); };
		auto indexOf = [&](int k) { for (int i = 0; i < n; ++i) if (key[i] == k) return i; return -1; };
		int path[MAXN]; int pn = g_lastLookup >= 0 ? itemDB.pathTo(g_lastLookup, path, MAXN) : 0;
		auto onPath = [&](int k) { for (int i = 0; i < pn; ++i) if (path[i] == k) return true; return false; };
		for (int i = 0; i < n; ++i) {                       // edges first
			int pi = indexOf(parent[i]); if (pi < 0) continue;
			bool lit = onPath(key[i]) && onPath(parent[i]);
			seg(at(pi), at(i), lit ? ACCENT : BOX_EDGE);
		}
		int found = 0, read = 0;
		for (int i = 0; i < n; ++i) {
			const ItemRecord& r = rec[i];
			bool clue = ITEMS[r.id].isClue;
			if (r.collected) ++found;
			if (r.discovered && clue) ++read;
			bool lit = onPath(key[i]);
			sf::CircleShape c(RAD); c.setOrigin({ RAD, RAD }); c.setPosition(at(i));
			c.setFillColor(!r.collected ? BOX : clue ? WARM : GOOD);
			c.setOutlineColor(lit ? ACCENT : BOX_EDGE); c.setOutlineThickness(lit ? -2.f : -1.f); w->draw(c);
			text(Str::num(key[i]), 10, r.collected ? sf::Color(22, 22, 24) : MUTED, at(i).x, at(i).y - 7, true, 1);
		}
		float ly = t + treeH + 20;
		if (pn > 0) {
			label("LAST SEARCH", FAINT, M, ly);
			Str s = Str(ITEMS[g_lastLookup].name) + ":  ";
			for (int i = 0; i < pn; ++i) s += Str::num(path[i]) + (i + 1 < pn ? " \xE2\x86\x92 " : "");
			std::snprintf(b, sizeof b, "   (%d step%s)", pn, pn == 1 ? "" : "s");
			float x = M + 104 + text(s, 11, BODY, M + 104, ly - 1);
			text(b, 11, FAINT, x, ly - 1);
		}
		else { label("LAST SEARCH", FAINT, M, ly); text("lights up when you pick something up", 11, FAINT, M + 104, ly - 1); }
		ly += 24;
		float lx = M;
		auto legend = [&](sf::Color c, const Str& s) {
			sf::CircleShape d(5.f); d.setOrigin({ 5.f, 5.f }); d.setPosition({ lx + 5, ly + 7 }); d.setFillColor(c);
			d.setOutlineColor(BOX_EDGE); d.setOutlineThickness(-1); w->draw(d);
			lx += 16 + text(s, 11, MUTED, lx + 16, ly) + 24;
		};
		std::snprintf(b, sizeof b, "Found %d of %d", found, n); legend(GOOD, b);
		std::snprintf(b, sizeof b, "Clues read %d", read); legend(WARM, b);
		legend(BOX, "Not yet");
	}

	// ================= BFS / DFS: finding the way =================
	{
		Str route[MAX_ROOMS]; int rn = 0;
		bool hasRoute = !hintText.empty() && lastRouteLen > 1;
		if (hasRoute) for (int i = 0; i < lastRouteLen && rn < MAX_ROOMS; ++i) route[rn++] = ROOMS[lastRoute[i]].name;
		int rows = hasRoute ? flow(route, rn, 0, false, rn - 1, false) : 1;
		bool open = cardOpen[4];
		std::snprintf(b, sizeof b, hasRoute ? "%d rooms" : "", rn);
		float t = section((open ? rows : 1) * ROW - 8 + 30, "BFS  /  DFS", "Finding the way",
		                  "BFS finds the shortest route; DFS finds rooms behind secret doors.", b, 4, rows > 1);
		if (hintText.empty()) text("The route appears with the hint (press H).", 12, FAINT, M, t + 4);
		else if (targetRoom == room) { std::snprintf(b, sizeof b, "The goal is in this room \xE2\x80\x94 %d steps of blood.", trailLen); text(b, 12, BODY, M, t + 4); }
		else if (lastRouteLen <= 1) text("No open route yet \xE2\x80\x94 something still blocks the way.", 12, FAINT, M, t + 4);
		else if (open || rows == 1) flow(route, rn, t, true, rn - 1, false);
		else {
			float x = M;
			for (int i = 0; i < rn; ++i) {
				float wd = nodeW(route[i]);
				if (x + wd + 36 > R) { text("...", 12, MUTED, x, t + 5); break; }
				node(x, t, route[i], i == rn - 1); x += wd;
				if (i + 1 < rn) { arrow(x + 5, t + NODE_H / 2, GAP - 10, LINK); x += GAP; }
			}
		}
		float dy = t + (open ? rows : 1) * ROW + 6;
		label("DFS", FAINT, M, dy + 1);
		if (lastDfsLen == 0) text("hidden rooms not searched yet \xE2\x80\x94 read the Hymn Page", 11, FAINT, M + 40, dy);
		else { std::snprintf(b, sizeof b, "searched %d rooms, secret doors included", lastDfsLen); text(b, 11, MUTED, M + 40, dy); }
	}

	// ---- scroll bar
	g_consoleContentH = y;
	w->setView(fixedV);
	if (g_consoleContentH > PH) {
		float trackH = PH - 24, barH = maxOf(40.f, trackH * PH / g_consoleContentH);
		float barY = 12 + (trackH - barH) * g_consoleScroll / (g_consoleContentH - PH);
		rrect(pw - 8, barY, 3, barH, 1.5f, sf::Color(70, 70, 76));
	}
	w->setView(uiView(*w));
}

