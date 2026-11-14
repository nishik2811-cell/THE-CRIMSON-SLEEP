#include <cstdint>
#include "screens.h"
#include "ds/MinMax.h"
#include "audio.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>

namespace {
const sf::Color CRIMSON(176, 30, 58), BONE(236, 224, 204), ASH(160, 146, 140);
float frand(float a, float b) { return a + (b - a) * (std::rand() / (float)RAND_MAX); }
bool fileExists(const Str& p) { FILE* f = std::fopen(p.c_str(), "rb"); if (f) std::fclose(f); return f != nullptr; }
bool anyKey() { return key_any(); }
// Gradient rectangle (corner colours: top-left, top-right, bottom-right, bottom-left).
void quad(sf::RenderWindow* w, float x0, float y0, float x1, float y1, sf::Color tl, sf::Color tr, sf::Color br, sf::Color bl,
	const sf::RenderStates& st = sf::RenderStates::Default) {
	sf::VertexArray v(sf::PrimitiveType::TriangleStrip, 4);
	v[0] = sf::Vertex{ { x0, y0 }, tl }; v[1] = sf::Vertex{ { x1, y0 }, tr };
	v[2] = sf::Vertex{ { x0, y1 }, bl }; v[3] = sf::Vertex{ { x1, y1 }, br };
	w->draw(v, st);
}
}

Screens::Screens() {
	if (jumpTex.loadFromFile(asset("graphics/jumpscare.png"))) { jump.setTexture(jumpTex, true); jump.setOrigin({ jumpTex.getSize().x / 2.f, jumpTex.getSize().y / 2.f }); }
	(void)fontTitle.openFromFile(asset("fonts/GrenzeGotisch.ttf"));
	(void)fontSerif.openFromFile(asset("fonts/CormorantGaramond.ttf"));
	(void)fontItalic.openFromFile(asset("fonts/CormorantGaramond-Italic.ttf"));
	// Your own backdrop wins if present; otherwise the generated blood moon.
	const char* bgs[] = { "graphics/title_bg.jpg", "graphics/title_bg.png", "graphics/title_bg_generated.png" };
	for (const char* b : bgs) if (fileExists(asset(b)) && bgTex.loadFromFile(asset(b))) break;
	bgTex.setSmooth(true);
	bg.setTexture(bgTex, true);
	hasLogo = logoTex.loadFromFile(asset("graphics/title_logo.png"));
	hasDawn = dawnTex.loadFromFile(asset("graphics/ending_true.jpg"));
	if (hasDawn) { dawnTex.setSmooth(true); dawnSprite.setTexture(dawnTex, true); }
	if (samTex.loadFromFile(asset("graphics/wraith.png"))) { sam.setTexture(samTex, true); sam.setTextureRect(sf::IntRect({ 0, 0 }, { 24, 48 })); sam.setOrigin({ 12.f, 48.f }); }
	logoTex.setSmooth(true);
	logo.setTexture(logoTex, true);
	for (Mote& m : motes) m = Mote{ frand(0, 1024), frand(0, 768), frand(-8, 8), frand(-22, -6), frand(0.8f, 2.4f), frand(60, 200) };
	for (int i = 0; i < 10; ++i) souls[i] = Soul{ 140 + i * 78.f + frand(-20, 20), 820 + frand(0, 260), frand(0, 6), frand(28, 45) };
	startBoot();
}

void Screens::startBoot() {
	slides.clear();
	slides.enqueue(Slide{ "This dream contains blood, jump scares and flashing lights.", "Headphones on. Lights off.", 3.5f, false });
	slides.enqueue(Slide{ "", "", 4.0f, true });
	slideT = 0;
}

void Screens::drawBackdrop(sf::RenderWindow* w, sf::Color tint) {
	// The backdrop fills the whole window (a wide screen too); the screens'
	// text and menus are then drawn in the 4:3 area in the middle.
	sf::View fv = fullView(*w);
	w->setView(fv);
	sf::FloatRect R({ fv.getCenter() - fv.getSize() / 2.f }, fv.getSize());
	// Slow "breathing" zoom so the scene never feels still.
	float s = 1.03f + 0.02f * std::sin(t * 0.25f);
	sf::Vector2u sz = bgTex.getSize();
	float base = maxOf(R.size.x / sz.x, R.size.y / sz.y);
	bg.setOrigin({ (float)(sz.x / 2.f), (float)(sz.y / 2.f) });
	bg.setScale({ (float)(base * s), (float)(base * s) });
	bg.setPosition({ (float)(512 + 6 * std::sin(t * 0.13f)), (float)(384) });
	bg.setColor(tint);
	w->draw(bg);
	// Low crimson fog bands drifting across the water.
	float fogW = maxOf(1400.f, R.size.x + 200.f);
	for (int i = 0; i < 3; ++i) {
		sf::RectangleShape fog(sf::Vector2f(fogW, 70));
		float x = R.position.x + std::fmod(t * (8 + i * 5) + i * 300, fogW) - fogW;
		for (int k = 0; k < 2; ++k) {
			fog.setPosition({ (float)(x + k * fogW), (float)(540 + i * 55 + 8 * std::sin(t * 0.4f + i)) });
			fog.setFillColor(sf::Color(90, 6, 18, 22));
			w->draw(fog);
		}
	}
	w->setView(uiView(*w));
}

void Screens::drawMotes(sf::RenderWindow* w, float dt, sf::Color c, bool rising) {
	for (Mote& m : motes) {
		m.x += m.vx * dt + std::sin(t + m.y * 0.02f) * 6 * dt;
		m.y += (rising ? m.vy : -m.vy * 0.7f) * dt;
		if (m.y < -10) m.y = 778;
		if (m.y > 778) m.y = -10;
		if (m.x < -10) m.x = 1034;
		if (m.x > 1034) m.x = -10;
		sf::CircleShape d(m.r);
		d.setPosition({ (float)(m.x), (float)(m.y) });
		d.setFillColor(sf::Color(c.r, c.g, c.b, (std::uint8_t)(m.a * (0.6f + 0.4f * std::sin(t * 2 + m.x)))));
		w->draw(d);
	}
}

void Screens::drawLogo(sf::RenderWindow* w, float x, float y, float height, float alpha) {
	std::uint8_t a = (std::uint8_t)(255 * alpha);   // steady - no flicker
	if (hasLogo) {
		float k = height / logoTex.getSize().y;
		// soft blood glow behind the lettering
		logo.setScale({ (float)(k * 1.01f), (float)(k * 1.01f) });
		logo.setPosition({ (float)(x - 2), (float)(y - 2) });
		logo.setColor(sf::Color(255, 40, 60, (std::uint8_t)(a * 0.25f)));
		w->draw(logo, sf::BlendAdd);
		logo.setScale({ (float)(k), (float)(k) });
		logo.setPosition({ (float)(x), (float)(y) });
		logo.setColor(sf::Color(255, 255, 255, a));
		w->draw(logo);
	}
	else {
		const char* words[] = { "The", "Crimson", "Sleep" };
		for (int i = 0; i < 3; ++i) {
			sf::Text tx(fontTitle, words[i], (unsigned)(height / 3.2f));
			tx.setFillColor(sf::Color(CRIMSON.r, CRIMSON.g, CRIMSON.b, a));
			tx.setPosition({ (float)(x + 40 * (i == 1 ? 0 : 1)), (float)(y + i * height / 3.1f) });
			w->draw(tx);
		}
	}
}

void Screens::centered(sf::RenderWindow* w, const sf::Font& f, const Str& s, unsigned size, sf::Color c, float y, float cx) {
	sf::Text tx(f, s, size);
	tx.setFillColor(c);
	sf::FloatRect b = tx.getLocalBounds();
	tx.setPosition({ (float)(std::floor(cx - b.size.x / 2 - b.position.x)), (float)(std::floor(y)) });
	w->draw(tx);
}

// Keyboard (Up/Down/W/S + Enter) and mouse menu.  Returns chosen index or -1.
int Screens::menu(sf::RenderWindow* w, const char* const* items, int n, float cx, float y, float size) {
	int before = menuIndex;
	if (menuInput && (key_hit(sf::Keyboard::Key::Up) || key_hit(sf::Keyboard::Key::W))) menuIndex = (menuIndex + n - 1) % n;
	if (menuInput && (key_hit(sf::Keyboard::Key::Down) || key_hit(sf::Keyboard::Key::S))) menuIndex = (menuIndex + 1) % n;
	if (menuIndex >= n) menuIndex = 0;
	sf::Vector2f mouse = w->mapPixelToCoords(sf::Mouse::getPosition(*w));
	int chosen = -1;
	for (int i = 0; i < n; ++i) {
		bool sel = i == menuIndex;
		Str s = sel ? Str("\xE2\x80\x94  ") + items[i] + "  \xE2\x80\x94" : items[i];
		sf::Text tx(fontSerif, sf::String::fromUtf8(s.begin(), s.end()), (unsigned)size);
		sf::FloatRect b = tx.getLocalBounds();
		tx.setPosition({ (float)(std::floor(cx - b.size.x / 2 - b.position.x)), (float)(std::floor(y + i * size * 1.25f)) });
		if (menuInput && tx.getGlobalBounds().contains(mouse)) {
			menuIndex = i;
			if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && !mouseWasDown) chosen = i;   // the press, not a held button
		}
		float glow = sel ? 0.85f + 0.15f * std::sin(t * 3) : 1.f;
		sf::Color ink = sel ? sf::Color((std::uint8_t)(BONE.r * glow), (std::uint8_t)(BONE.g * glow), (std::uint8_t)(BONE.b * glow)) : ASH;
		ink.a = (std::uint8_t)(255 * menuAlpha);
		tx.setFillColor(ink);
		w->draw(tx);
	}
	if (menuInput && (key_hit(sf::Keyboard::Key::Enter) || key_hit(sf::Keyboard::Key::Space))) chosen = menuIndex;
	mouseWasDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	if (g_audio && menuIndex != before) g_audio->play(SFX_UI_MOVE, 60);
	if (g_audio && chosen >= 0) g_audio->play(SFX_UI_CLICK, 70);
	return chosen;
}

// ------------------------------------------------------------------ boot
int Screens::boot(sf::RenderWindow* w, float dt) {
	t += dt; slideT += dt;
	w->setView(uiView(*w));
	Slide s;
	if (!slides.front(&s)) return ST_TITLE;
	float fade = slideT < 0.8f ? slideT / 0.8f : (s.seconds - slideT < 0.8f ? (s.seconds - slideT) / 0.8f : 1.f);
	if (fade < 0) fade = 0;
	float logoY = 140 - 10 * minOf(slideT / s.seconds, 1.f), logoA = slideT < 1.5f ? slideT / 1.5f : 1.f;
	if (s.logo) {
		// Logo emerges from the dark over the backdrop. It does NOT fade out:
		// the title screen picks it up exactly here and glides it into place.
		float in = minOf(slideT / 0.8f, 1.f);
		drawBackdrop(w, sf::Color((std::uint8_t)(255 * in), (std::uint8_t)(255 * in), (std::uint8_t)(255 * in)));
		drawMotes(w, dt, sf::Color(200, 40, 50), true);
		drawLogo(w, 512 - logoWidth(420) / 2, logoY, 420, logoA);
	}
	else {
		centered(w, fontItalic, s.line1, 30, sf::Color(200, 180, 172, (std::uint8_t)(255 * fade)), 340);
		if (s.line2[0]) centered(w, fontSerif, s.line2, 24, sf::Color(176, 30, 58, (std::uint8_t)(255 * fade)), 390);
	}
	if (slideT >= s.seconds || anyKey()) {
		slides.dequeue(); slideT = 0;
		if (s.logo) { introT = 0; introFromY = logoY; introFromA = logoA; }   // hand over to the title
	}
	return slides.empty() ? ST_TITLE : ST_BOOT;
}

// ----------------------------------------------------------------- title
int Screens::title(sf::RenderWindow* w, float dt) {
	t += dt;
	// Hand-over from the boot logo: k eases 0 -> 1 as the logo glides to the
	// left, the shade comes in behind it, then the tagline and menu fade up.
	const float GLIDE = 1.3f, FADE_IN = 0.7f;
	float k = 1, ui = 1;
	bool skipped = false;
	if (introT >= 0) {
		introT += dt;
		if (key_any() && introT > 0.15f) { introT = GLIDE + FADE_IN; skipped = true; }   // a key finishes it (and isn't also a menu pick)
		float g = minOf(introT / GLIDE, 1.f);
		k = g * g * (3 - 2 * g);                                      // smoothstep
		ui = clampTo((introT - GLIDE * 0.6f) / FADE_IN, 0.f, 1.f);
		if (introT >= GLIDE + FADE_IN) introT = -1;
	}
	drawBackdrop(w);
	// Darken the left third so the logo and menu read clearly (as in the Godot build).
	std::uint8_t shade = (std::uint8_t)(200 * k);
	{
		sf::View fv = fullView(*w); w->setView(fv);
		float left = fv.getCenter().x - fv.getSize().x / 2, top = fv.getCenter().y - fv.getSize().y / 2;
		sf::RectangleShape edge({ maxOf(0.f, -left), fv.getSize().y }); edge.setPosition({ left, top });
		edge.setFillColor(sf::Color(0, 0, 0, shade)); w->draw(edge);
		quad(w, 0, top, 620, top + fv.getSize().y, sf::Color(0, 0, 0, shade), sf::Color(0, 0, 0, 0), sf::Color(0, 0, 0, 0), sf::Color(0, 0, 0, shade));
		w->setView(uiView(*w));
	}
	drawMotes(w, dt, sf::Color(200, 40, 50), true);

	const float cx = 290;
	float logoH = 420 + (330 - 420) * k;
	float lx0 = 512 - logoWidth(420) / 2, lx1 = cx - logoWidth(330) / 2;
	drawLogo(w, lx0 + (lx1 - lx0) * k, introFromY + (70 - introFromY) * k, logoH, introFromA + (1 - introFromA) * k);
	centered(w, fontItalic, "some dreams don't let you wake up", 24, sf::Color(222, 206, 196, (std::uint8_t)(255 * ui)), 424, cx);
	menuAlpha = ui; menuInput = ui >= 1 && !skipped;
	// "Continue" only appears when there is a saved run.
	const char* withSave[] = { "Continue the dream", "Begin the dream", "Watch the dream", "Credits", "Controls", "Wake up" };
	const char* noSave[] = { "Begin the dream", "Watch the dream", "Credits", "Controls", "Wake up" };
	int n = hasSave ? 6 : 5;
	if (watchMenu) {                                     // Watch the dream: pick which of the four endings to see
		const char* endings[] = { "The true ending", "Escape", "The secret way out", "Trapped", "Back" };
		int e = menu(w, endings, 5, cx, 456, 28);
		menuAlpha = 1; menuInput = true;
		if (key_hit(sf::Keyboard::Key::Escape) || e == 4) { watchMenu = false; menuIndex = hasSave ? 2 : 1; return ST_TITLE; }
		if (e >= 0) { demoChoice = e; watchMenu = false; menuIndex = 0; return ST_DEMO; }
		return ST_TITLE;
	}
	int c = menu(w, hasSave ? withSave : noSave, n, cx, 456, 30);
	menuAlpha = 1; menuInput = true;
	if (c < 0) return ST_TITLE;
	if (!hasSave) ++c;                       // same numbering either way
	if (c == 0) return ST_CONTINUE;
	if (c == 1) return ST_NEW_GAME;
	if (c == 2) { watchMenu = true; menuIndex = 0; return ST_TITLE; }
	if (c == 3) { menuIndex = 0; startCredits(); return ST_CREDITS; }
	if (c == 4) { menuIndex = 0; return ST_CONTROLS; }
	if (c == 5) w->close();
	return ST_TITLE;
}

int Screens::controls(sf::RenderWindow* w, float dt) {
	t += dt;
	drawBackdrop(w, sf::Color(110, 90, 90));
	// The panel: two columns on a fixed grid - the keys on the left (each key
	// in its own little cap, every action starting on the same line), the
	// survival rules on the right, wrapped to the column so nothing overflows.
	const float PX = 72, PY = 64, PW = 880, PH = 640;
	sf::RectangleShape p(sf::Vector2f(PW, PH)); p.setPosition({ PX, PY }); p.setFillColor(sf::Color(8, 2, 4, 222));
	p.setOutlineColor(sf::Color(120, 6, 18)); p.setOutlineThickness(2); w->draw(p);
	centered(w, fontTitle, "Controls", 44, CRIMSON, PY + 14);
	const float TOP = PY + 96;                         // where both columns start
	const float LX = PX + 44, CAP_W = 132, AX = LX + CAP_W + 18;   // key caps, then the action text
	const float RX = PX + 500, RW = PX + PW - 40 - RX;  // the right column
	sf::RectangleShape div({ 1.f, PH - 170 }); div.setPosition({ RX - 28, TOP - 6 }); div.setFillColor(sf::Color(120, 6, 18, 160)); w->draw(div);
	auto label = [&](const sf::Font& f, const char* s, unsigned size, sf::Color c, float x, float y) {
		sf::Text tx(f, s, size); tx.setFillColor(c); tx.setPosition({ std::floor(x), std::floor(y) }); w->draw(tx); return tx.getLocalBounds().size.x;
	};
	label(fontItalic, "Keys", 20, sf::Color(220, 170, 170), LX, TOP - 34);
	label(fontItalic, "How to survive", 20, sf::Color(220, 170, 170), RX, TOP - 34);

	struct Row { const char* key; const char* what; };
	static const Row ROWS[] = {
		{ "Arrows / WASD", "walk" },
		{ "E / Enter", "interact, open doors" },
		{ "F", "flare the lantern" },
		{ "L", "lantern on / off" },
		{ "Q", "stomp a spider" },
		{ "Shift", "strike with the sword" },
		{ "C  (hold)", "block with the sword" },
		{ "B", "go back one room" },
		{ "H", "ask the house for a hint" },
		{ "I", "inventory" },
		{ "J", "notes on the dead" },
		{ "Tab", "live data-structure console" },
		{ "M", "mute" },
		{ "F11", "fullscreen / window" },
		{ "Space", "dismiss text" },
		{ "Esc", "pause" },
	};
	const float ROW_H = 30;
	for (int i = 0; i < (int)(sizeof ROWS / sizeof ROWS[0]); ++i) {
		float y = TOP + i * ROW_H;
		sf::Text kt(fontSerif, ROWS[i].key, 17);
		float tw = kt.getLocalBounds().size.x, kw = tw + 18 > 32 ? tw + 18 : 32;   // single letters get an even, square-ish cap
		sf::RectangleShape cap({ kw, 23 }); cap.setPosition({ LX, y });
		cap.setFillColor(sf::Color(40, 10, 16, 200)); cap.setOutlineColor(sf::Color(120, 40, 50)); cap.setOutlineThickness(1); w->draw(cap);
		label(fontSerif, ROWS[i].key, 17, BONE, LX + (kw - tw) / 2 - kt.getLocalBounds().position.x, y + 1);
		label(fontSerif, ROWS[i].what, 19, sf::Color(214, 202, 190), AX, y);
	}

	// The rules, each wrapped to the column width, with a small gap between them.
	static const char* TIPS[] = {
		"Samara hunts by sound. Stand still until she loses you, or flare.",
		"Never turn your back on Carrie. Face her to freeze her.",
		"Spirits walk fixed paths: watch them, then cross.",
		"A spider at your feet: stomp it (Q). Lantern off, it can barely see you.",
		"The Devil's blade glints white just before it falls: hold C, facing him.",
		"Fear kills too: keep the lantern lit and rest.",
		"Stuck for 30 seconds? A trail of blood appears.",
		"Some doors open only to knowledge, not keys.",
	};
	float y = TOP;
	for (const char* tip : TIPS) {
		// greedy word wrap into the column
		Str word, line, out;
		auto fits = [&](const Str& s) { sf::Text m(fontItalic, s, 19); return m.getLocalBounds().size.x <= RW - 16; };
		const char* c = tip;
		while (true) {
			if (*c == ' ' || *c == 0) {
				Str test = line.empty() ? word : line + " " + word;
				if (!line.empty() && !fits(test)) { out += line + "\n"; line = word; }
				else line = test;
				word.clear();
				if (*c == 0) break;
			}
			else word += *c;
			++c;
		}
		out += line;
		label(fontItalic, "-", 19, sf::Color(176, 30, 58), RX, y);
		sf::Text tt(fontItalic, out, 19); tt.setFillColor(sf::Color(220, 190, 186)); tt.setLineSpacing(1.05f);
		tt.setPosition({ RX + 16, std::floor(y) }); w->draw(tt);
		y += tt.getLocalBounds().size.y + tt.getLocalBounds().position.y + 12;
	}

	centered(w, fontItalic, "Enter or Esc to return", 20, ASH, PY + PH - 40);
	if (key_hit(sf::Keyboard::Key::Enter) || key_hit(sf::Keyboard::Key::Escape) || key_hit(sf::Keyboard::Key::Space)) return -100; // caller decides
	return ST_CONTROLS;
}

int Screens::pause(sf::RenderWindow* w, float dt) {
	t += dt;
	drawBackdrop(w, sf::Color(90, 70, 70));
	drawLogo(w, 512 - 150, 80, 230, 0.8f);
	const char* items[] = { "Return to the dream", "Save & quit to menu", "Controls", "Leave to the title", "Wake up" };
	int c = menu(w, items, 5, 512, 370, 34);
	if (key_hit(sf::Keyboard::Key::Escape)) return ST_PLAYING;
	if (c == 0) return ST_PLAYING;
	if (c == 1) { menuIndex = 0; return ST_SAVE_QUIT; }
	if (c == 2) { menuIndex = 0; return ST_CONTROLS; }
	if (c == 3) { menuIndex = 0; return ST_TITLE; }
	if (c == 4) w->close();
	return ST_PAUSED;
}

int Screens::retry(sf::RenderWindow* w, float dt, const Game& g) {
	t += dt;
	deathT += dt;
	w->setView(uiView(*w));
	const float SCARE = 1.4f;
	if (deathT < SCARE) {
		// The face slams in: punch zoom, violent shake, red/cyan split, static, blood flash.
		sf::RectangleShape blk(sf::Vector2f(1024, 768)); blk.setFillColor(sf::Color::Black); w->draw(blk);
		float age = deathT;
		float punch = age < 0.12f ? 0.55f + age / 0.12f * 0.6f : 1.15f + 0.05f * std::sin(age * 40);
		float sx = 1024.f / jumpTex.getSize().x * punch, sy = 768.f / jumpTex.getSize().y * punch;
		float jit = age < 0.6f ? 18.f : 8.f;
		sf::Vector2f c(512 + (std::rand() % 21 - 10) * jit / 10, 384 + (std::rand() % 21 - 10) * jit / 10);
		std::uint8_t al = (std::uint8_t)(255 * (age < SCARE - 0.3f ? 1 : (SCARE - age) / 0.3f));
		jump.setScale({ sx, sy });
		jump.setPosition({ c.x - 7, c.y }); jump.setColor(sf::Color(255, 0, 0, (std::uint8_t)(al * 0.3f))); w->draw(jump, sf::BlendAdd);
		jump.setPosition({ c.x + 7, c.y + 2 }); jump.setColor(sf::Color(0, 120, 120, (std::uint8_t)(al * 0.2f))); w->draw(jump, sf::BlendAdd);
		jump.setPosition(c); jump.setColor(sf::Color(255, 255, 255, al)); w->draw(jump);
		for (int i = 0; i < 40; ++i) {
			sf::RectangleShape ln(sf::Vector2f(1024, (float)(1 + std::rand() % 3)));
			ln.setPosition({ 0.f, (float)(std::rand() % 768) });
			ln.setFillColor(sf::Color(255, 200, 200, (std::uint8_t)((std::rand() % 50) * al / 255)));
			w->draw(ln);
		}
		if (age < 0.09f || (age > 0.2f && age < 0.25f)) { sf::RectangleShape f(sf::Vector2f(1024, 768)); f.setFillColor(sf::Color(200, 0, 20, 110)); w->draw(f); }
		return ST_RETRY;                                    // no menu input during the scare
	}
	float fadeIn = minOf(1.f, (deathT - SCARE) / 0.6f);
	sf::RectangleShape r(sf::Vector2f(1024, 768)); r.setFillColor(sf::Color(18, 0, 3)); w->draw(r);
	drawMotes(w, dt, sf::Color(150, 10, 20), false);
	bool mind = g.deathTip == TIP_FEAR;
	centered(w, fontTitle, mind ? "Your Mind Breaks" : "You Died", 86, CRIMSON, 150);
	centered(w, fontItalic, g.deathReason, 28, BONE, 270);
	centered(w, fontSerif, Str("Remember - ") + TIPS[g.deathTip].title + ":", 22, ASH, 330);
	Str body = TIPS[g.deathTip].body;
	int nl = body.find('\n');
	Str first = nl == Str::npos ? body : body.substr(nl + 1);
	nl = first.find('\n'); if (nl != Str::npos) first = first.substr(0, nl);
	centered(w, fontSerif, first, 22, sf::Color(220, 170, 170), 360);
	const char* items[] = { "Dream again", "Leave to the title" };
	int c = menu(w, items, 2, 512, 460, 36);
	if (fadeIn < 1.f) {                                     // fade in from black after the scare
		sf::RectangleShape veil(sf::Vector2f(1024, 768)); veil.setFillColor(sf::Color(0, 0, 0, (std::uint8_t)(255 * (1 - fadeIn)))); w->draw(veil);
		return ST_RETRY;
	}
	if (c == 0) return ST_PLAYING;
	if (c == 1) { menuIndex = 0; return ST_TITLE; }
	return ST_RETRY;
}

// ---------------------------------------------------------------- ending
int Screens::ending(sf::RenderWindow* w, float dt, const RunStats& st) {
	t += dt; endT += dt;
	w->setView(uiView(*w));
	const int e = ending_type;
	struct Script { const char* lines[6]; const char* name; };
	static const Script scripts[] = {
		{ { "" }, "" },
		{ { "The Devil is dust, and the house lets you go.", "You stagger out of the front door into a cold, colourless morning.", "Behind you, faint and far away, a woman is still weeping.", "Somewhere beneath the chapel, something was left undone.", "" }, "ESCAPE ENDING" },
		{ { "The petrol catches. The house screams.", "Smoke fills the corridors - every door slams shut.", "Through the flames they come, all ten of them.", "The fire burns for three days.", "No one finds you. The house simply... sleeps again.", "" }, "TRAPPED ENDING" },
		{ { "You run, the locket burning in your fist.", "The front door gives way to a forest of black trees.", "Samara walks beside you now - quiet, at peace.", "But beneath the house, the Devil still sleeps.", "And one day, he will dream of you.", "" }, "SECRET ENDING" },
		{ { "The last blow falls. The Devil bursts into ash, and is gone.", "For a moment there is only silence.", "Then, far above, the weeping stops - and Samara is at peace.", "Morning light finds its way into the house at last.", "For the first time since June 26, 1953,", "the house sleeps without dreaming." }, "TRUE ENDING" },
	};
	const Script& sc = scripts[e < 1 || e > 4 ? 1 : e];

	// Sky: the crimson moon slowly gives way to dawn on the good endings,
	// stays red for the secret one, turns to fire for the trapped one.
	float dawn = (e == END_TRUE ? 1.f : e == END_ESCAPE ? 0.45f : 0.f) * minOf(1.f, endT / 14.f);
	sf::Color tint(255, (std::uint8_t)(255 * (0.6f + 0.4f * dawn)), (std::uint8_t)(255 * (0.55f + 0.45f * dawn)));
	if (e == END_TRUE && hasDawn) {
		// The true ending: the crimson sun rising over the mountains and the
		// fog - a slow push in, brightening out of the dark.
		sf::Vector2u sz = dawnTex.getSize();
		sf::View fv = fullView(*w); w->setView(fv);
		float s = maxOf(fv.getSize().x / sz.x, fv.getSize().y / sz.y) * (1.02f + 0.03f * minOf(1.f, endT / 30.f));
		dawnSprite.setOrigin({ sz.x / 2.f, sz.y / 2.f }); dawnSprite.setScale({ s, s });
		dawnSprite.setPosition({ 512.f - 40.f * minOf(1.f, endT / 30.f), 384.f });
		std::uint8_t b = (std::uint8_t)(255 * minOf(0.62f, 0.3f + endT / 8.f));   // dulled, so the words stay clear
		dawnSprite.setColor(sf::Color(b, b, b));
		w->draw(dawnSprite);
		w->setView(uiView(*w));
	}
	else drawBackdrop(w, tint);
	if (dawn > 0 && !(e == END_TRUE && hasDawn)) {
		// Dawn: deep blue overhead, warm gold at the horizon, fading over the water.
		sf::Color top(40, 60, 110, (std::uint8_t)(120 * dawn)), mid(255, 190, 140, (std::uint8_t)(170 * dawn)), low(120, 70, 50, 0);
		sf::View fv = fullView(*w); w->setView(fv);
		float l = fv.getCenter().x - fv.getSize().x / 2, r = l + fv.getSize().x, tp = fv.getCenter().y - fv.getSize().y / 2, bt = tp + fv.getSize().y;
		quad(w, l, tp, r, 560, top, top, mid, mid, sf::BlendAdd);
		quad(w, l, 560, r, bt, mid, mid, low, low, sf::BlendAdd);
		w->setView(uiView(*w));
	}
	// Full-window overlays (they reach past the 4:3 area on a wide screen).
	sf::View FV = fullView(*w);
	const float FL = FV.getCenter().x - FV.getSize().x / 2, FT = FV.getCenter().y - FV.getSize().y / 2, FR = FL + FV.getSize().x;
	auto fullRect = [&](sf::Color c, const sf::RenderStates& st = sf::RenderStates::Default) {
		w->setView(FV);
		sf::RectangleShape r(FV.getSize()); r.setPosition({ FL, FT }); r.setFillColor(c); w->draw(r, st);
		w->setView(uiView(*w));
	};
	if (e == END_TRAPPED) {
		fullRect(sf::Color(255, 90, 20, (std::uint8_t)(50 + 30 * std::sin(endT * 9) + std::rand() % 20)), sf::BlendAdd);
		drawMotes(w, dt, sf::Color(255, 140, 40), true);
	}
	else drawMotes(w, dt, e == END_TRUE ? sf::Color(255, 230, 200) : sf::Color(200, 40, 50), true);

	// The true ending: a flock of birds crosses the sunrise - the first living
	// things the house has seen in seventy years.
	if (e == END_TRUE && endT > 5.f) {
		for (int i = 0; i < 9; ++i) {
			float bt = endT - 5.f - (i % 3) * 0.6f;
			if (bt < 0) continue;
			float x = -60.f + bt * (38.f + (i % 4) * 4.f) + (i / 3) * 28.f;
			float y = 250.f + (i % 3) * 22.f + (i / 3) * 14.f - bt * 4.f + 4.f * std::sin(bt * 1.3f + i);
			if (x > 1100) continue;
			float flap = std::sin(bt * (7.f + i % 3) + i) * 4.f;
			float sz = 5.f + (i % 3);
			sf::Color ink(30, 8, 16, (std::uint8_t)(220 * minOf(1.f, bt / 1.5f)));
			sf::Vertex wing[4] = { { { x - sz, y - flap }, ink }, { { x, y }, ink }, { { x, y }, ink }, { { x + sz, y - flap }, ink } };
			w->draw(wing, 4, sf::PrimitiveType::Lines);
		}
	}
	if (!(e == END_TRUE && hasDawn)) fullRect(sf::Color(0, 0, 0, 90));
	if (e == END_TRUE && hasDawn) {                     // a soft shade only behind the lines, so the sun stays golden
		w->setView(FV);
		quad(w, FL, 140, FR, 200, sf::Color(0, 0, 0, 0), sf::Color(0, 0, 0, 0), sf::Color(10, 0, 0, 110), sf::Color(10, 0, 0, 110));
		quad(w, FL, 200, FR, 480, sf::Color(10, 0, 0, 110), sf::Color(10, 0, 0, 110), sf::Color(0, 0, 0, 0), sf::Color(0, 0, 0, 0));
		w->setView(uiView(*w));
	}

	if ((e == END_TRUE || e == END_ESCAPE) && endT < 2.f) {   // out of the white of the Devil's death
		fullRect(sf::Color(255, 248, 240, (std::uint8_t)(255 * (1 - endT / 2.f))));
	}

	// Story lines appear one after another...
	int n = 0; while (n < 6 && sc.lines[n] && sc.lines[n][0]) ++n;
	const float per = 3.4f;
	for (int i = 0; i < n; ++i) {
		float lt = endT - 1.f - i * per;
		if (lt <= 0) continue;
		float a = minOf(1.f, lt / 1.2f);
		if (endT > 1.f + n * per + 2) a *= maxOf(0.f, 1 - (endT - (3.f + n * per)) / 1.5f);
		for (int k = 0; k < 4; ++k)                       // a soft dark shadow keeps the words readable over a bright sky
			centered(w, fontItalic, sc.lines[i], 30, sf::Color(20, 0, 6, (std::uint8_t)(110 * a)), 172 + i * 52.f + (k / 2) * 2 - 1, 512.f + (k % 2) * 3 - 1.5f);
		centered(w, fontItalic, sc.lines[i], 30, sf::Color(250, 238, 224, (std::uint8_t)(255 * a)), 170 + i * 52.f);
	}
	// ...then the verdict, run statistics and credits.
	float tail = endT - (4.5f + n * per);
	if (tail > 0) {
		float a = minOf(1.f, tail / 1.5f);
		if (e == END_TRUE) centered(w, fontTitle, "The Crimson Sleep is Over", 60, sf::Color(255, 210, 190, (std::uint8_t)(255 * a)), 110);
		else drawLogo(w, 512 - 120, 70, 190, a);
		centered(w, fontSerif, sc.name, 34, sf::Color(CRIMSON.r + 40, CRIMSON.g + 20, CRIMSON.b + 20, (std::uint8_t)(255 * a)), 300);
		int mins = (int)st.time / 60, secs = (int)st.time % 60;
		char b[256];
		std::snprintf(b, sizeof b, "Time %d:%02d      Rooms %d / 10      Items %d      Spirits banished %d      Deaths %d      Peak fear %d%%",
			mins, secs, st.roomsSeen, st.itemsFound, st.banished, st.deaths, (int)st.peakFear);
		centered(w, fontSerif, b, 21, sf::Color(220, 200, 190, (std::uint8_t)(255 * a)), 360);
		const char* rank = e == END_TRUE ? (st.deaths == 0 ? "Rank: Dreamwalker - flawless" : st.deaths < 3 ? "Rank: Lightbringer" : "Rank: Survivor")
			: e == END_SECRET ? "Rank: The One Who Ran" : e == END_ESCAPE ? "Rank: Half-Awake" : "Rank: Ash";
		centered(w, fontItalic, rank, 26, sf::Color(255, 200, 170, (std::uint8_t)(255 * a)), 400);
		if (e != END_TRUE) centered(w, fontItalic, "Hint: slay the Devil AND return Samara's locket...", 20, sf::Color(200, 150, 150, (std::uint8_t)(200 * a)), 440);

		// Credits roll.
		const char* credits[] = { "THE CRIMSON SLEEP", "", "A Retro Horror Adventure",
			"", "Created by Nishita, Yashieta, Anwita and Aryan",
			"Music and sound: the team, Alex Morgan and Pixabay artists", "", "", "Thank you for playing.", "Sleep well." };
		float cy = 768 - (tail - 3) * 34;
		if (tail > 3) for (int i = 0; i < 10; ++i) {
			float y = cy + i * 34;
			if (y < 480 || y > 700) continue;
			float fa = minOf(1.f, minOf((y - 480) / 60, (700 - y) / 40));
			centered(w, i == 0 ? fontTitle : fontSerif, credits[i], i == 0 ? 30 : 21, sf::Color(230, 215, 200, (std::uint8_t)(255 * fa)), y);
		}
		if (tail > 2) centered(w, fontItalic, "Enter to return to the title", 18, sf::Color(170, 150, 150, (std::uint8_t)(160 + 60 * std::sin(t * 3))), 738);
		if (tail > 2 && key_hit(sf::Keyboard::Key::Enter)) { menuIndex = 0; return ST_TITLE; }
	}
	if (key_hit(sf::Keyboard::Key::Escape)) { menuIndex = 0; return ST_TITLE; }
	return ST_ENDING;
}

// ------------------------------------------------------------------ credits
// Black screen, TV static, the credits rolling up like a film.  Sound: the
// horror sting first, under static; then the song builds in gradually while
// the sting fades and the static thins to a faint hiss.
void Screens::stopCredits() { crHorror.stop(); crSong.stop(); crStatic.stop(); crStarted = false; }

int Screens::credits(sf::RenderWindow* w, float dt) {
	if (!crStarted) {
		crStarted = true; crT = 0;
		if (crHorror.openFromFile(asset("audio/credits_horror.ogg"))) { crHorror.setVolume(0); crHorror.play(); }
		if (crStatic.openFromFile(asset("audio/static_loop.ogg"))) { crStatic.setLooping(true); crStatic.setVolume(0); crStatic.play(); }
		if (crSong.openFromFile(asset("audio/rank_music.ogg"))) { crSong.setLooping(true); crSong.setVolume(0); crSong.play(); }
	}
	crT += dt;
	auto ramp = [](float t, float t0, float t1, float a, float b) {        // a -> b between t0 and t1
		if (t <= t0) return a;
		if (t >= t1) return b;
		return a + (b - a) * (t - t0) / (t1 - t0);
	};
	crHorror.setVolume(crT < 12 ? ramp(crT, 0, 2.5f, 0, 90) : ramp(crT, 12, 22, 90, 0));
	crSong.setVolume(ramp(crT, 10, 32, 0, 80));                          // builds up slowly
	crStatic.setVolume(crT < 10 ? ramp(crT, 0, 3, 0, 14) : ramp(crT, 10, 36, 14, 4));   // a faint hiss under the music

	w->setView(uiView(*w));
	sf::RectangleShape black({ 1024.f, 768.f }); black.setFillColor(sf::Color::Black); w->draw(black);   // pitch black
	// static: fresh noise every other frame, stronger at the start
	static int tick = 0;
	if (++tick % 2 == 0) {
		for (unsigned y = 0; y < 192; ++y)
			for (unsigned x = 0; x < 256; ++x) { std::uint8_t v = (std::uint8_t)(std::rand() % 256); noiseImg.setPixel({ x, y }, sf::Color(v, v, v)); }
		noiseTex.update(noiseImg);
	}
	noise.setScale({ 1024.f / 256, 768.f / 192 });
	noise.setColor(sf::Color(255, 255, 255, (std::uint8_t)ramp(crT, 8, 36, 14, 5)));   // barely-there static
	w->draw(noise);

	static const char* const LINES[] = {
		"#THE CRIMSON SLEEP", "", "", "", "", "", "", "A 2D survival horror game", "", "",   // '#' = the butterfly logo (it is about 7 lines tall)
		"@CREATED BY", "Nishita", "Yashieta", "Anwita", "Aryan", "", "",
		"Design, code, art and sound - built for this project",
		"Every system and every data structure written by hand in C++17,", "on SFML 3 - with no STL.", "", "",
		"@THE DATA STRUCTURES",
		"World graph  -  BFS  -  DFS", "Haunting-event queue  -  threat priority queue (heap)",
		"Inventory linked list  -  clue binary search tree  -  room-history stack",
		"Tile, room and item arrays  -  hand-written strings", "A*  pathfinding for the Weavers' eight-state AI", "", "",
		"@THE DEAD", "Samara, the cursed bride", "Carrie, the blood oracle", "The restless spirits of June 26, 1953",
		"The Weavers", "The Devil beneath the crypt", "Alfred, who stayed too long", "", "",
		"@MUSIC", "Title Music  -  Ambient Horror (Alex Morgan)  -  Creep  -  Chase",
		"Wrong  -  Apartments  -  Moonlit Lullaby for the Lost", "", "",
		"@SOUND", "The team's sound library, and effects from Pixabay:",
		"freesound_community  -  judgecruz1 / remusic  -  Dragon Studio  -  hgoliya08", "", "",
		"@TYPE", "Grenze Gotisch  -  Cormorant Garamond  -  Nosifer  -  DejaVu Sans", "", "", "",
		"For anyone who has stayed awake beside someone they could not wake.", "", "", "",
	};
	const int N = (int)(sizeof LINES / sizeof LINES[0]);
	const float lineH = 36.f, speed = 30.f;
	float top = 790.f - crT * speed;
	for (int i = 0; i < N; ++i) {
		float y = top + i * lineH;
		if (LINES[i][0] == '#' && hasLogo) {                                     // the butterfly logo, rolling up with the text
			const float LH = 240.f;
			float ly = y - 10, lfade = minOf(1.f, minOf((ly + LH * 0.6f) / 140.f, (790 - ly) / 140.f));
			if (lfade > 0 && ly < 800) drawLogo(w, 512 - logoWidth(LH) / 2, ly, LH, lfade);
			continue;
		}
		if (y < -40 || y > 800) continue;
		const char* L = LINES[i];
		bool head = L[0] == '#', sub = L[0] == '@';
		if (head || sub) ++L;
		float fade = minOf(1.f, minOf((y + 30) / 140.f, (790 - y) / 140.f));       // fades in at the bottom, out at the top
		if (fade <= 0) continue;
		sf::Color c = head ? sf::Color(200, 30, 50) : sub ? sf::Color(170, 60, 70) : sf::Color(205, 192, 182);
		c.a = (std::uint8_t)(255 * fade);
		centered(w, head ? fontTitle : fontSerif, L, head ? 48u : sub ? 17u : 22u, c, y);
	}
	float endY = top + N * lineH;
	if (endY < 420) {                                                        // the roll has finished
		float a = minOf(1.f, (420 - endY) / 160.f);
		centered(w, fontItalic, "Thank you for playing.", 30, sf::Color(220, 205, 195, (std::uint8_t)(255 * a)), 340);
		centered(w, fontSerif, "Esc / Enter - back", 16, sf::Color(150, 120, 120, (std::uint8_t)(200 * a)), 420);
	}
	if (key_hit(sf::Keyboard::Key::Escape) || key_hit(sf::Keyboard::Key::Enter) || key_hit(sf::Keyboard::Key::Space)) { stopCredits(); return ST_TITLE; }
	return ST_CREDITS;
}
