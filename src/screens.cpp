#include "screens.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>

namespace {
const sf::Color CRIMSON(176, 30, 58), BONE(236, 224, 204), ASH(160, 146, 140);
float frand(float a, float b) { return a + (b - a) * (std::rand() / (float)RAND_MAX); }
bool fileExists(const std::string& p) { FILE* f = std::fopen(p.c_str(), "rb"); if (f) std::fclose(f); return f != nullptr; }
bool anyKey() {
	for (int k = 0; k < sf::Keyboard::KeyCount; ++k) if (key_hit((sf::Keyboard::Key)k)) return true;
	return false;
}
}

Screens::Screens() {
	fontTitle.loadFromFile(asset("fonts/GrenzeGotisch.ttf"));
	fontSerif.loadFromFile(asset("fonts/CormorantGaramond.ttf"));
	fontItalic.loadFromFile(asset("fonts/CormorantGaramond-Italic.ttf"));
	// Your own backdrop wins if present; otherwise the generated blood moon.
	const char* bgs[] = { "graphics/title_bg.jpg", "graphics/title_bg.png", "graphics/title_bg_generated.png" };
	for (const char* b : bgs) if (fileExists(asset(b)) && bgTex.loadFromFile(asset(b))) break;
	bgTex.setSmooth(true);
	bg.setTexture(bgTex, true);
	hasLogo = logoTex.loadFromFile(asset("graphics/title_logo.png"));
	logoTex.setSmooth(true);
	logo.setTexture(logoTex, true);
	for (Mote& m : motes) m = Mote{ frand(0, 1024), frand(0, 768), frand(-8, 8), frand(-22, -6), frand(0.8f, 2.4f), frand(60, 200) };
	for (int i = 0; i < 10; ++i) souls[i] = Soul{ 140 + i * 78.f + frand(-20, 20), 820 + frand(0, 260), frand(0, 6), frand(28, 45) };
	startBoot();
}

void Screens::startBoot() {
	slides.clear();
	slides.enqueue(Slide{ "a Data Structures Laboratory project", "", 3.0f, false });
	slides.enqueue(Slide{ "This dream contains blood, jump scares and flashing lights.", "Headphones on. Lights off.", 3.5f, false });
	slides.enqueue(Slide{ "", "", 4.0f, true });
	slideT = 0;
}

void Screens::drawBackdrop(sf::RenderWindow* w, sf::Color tint) {
	w->setView(w->getDefaultView());
	// Slow "breathing" zoom so the scene never feels still.
	float s = 1.03f + 0.02f * std::sin(t * 0.25f);
	sf::Vector2u sz = bgTex.getSize();
	float base = std::max(1024.f / sz.x, 768.f / sz.y);
	bg.setOrigin(sz.x / 2.f, sz.y / 2.f);
	bg.setScale(base * s, base * s);
	bg.setPosition(512 + 6 * std::sin(t * 0.13f), 384);
	bg.setColor(tint);
	w->draw(bg);
	// Low crimson fog bands drifting across the water.
	for (int i = 0; i < 3; ++i) {
		sf::RectangleShape fog(sf::Vector2f(1400, 70));
		float x = std::fmod(t * (8 + i * 5) + i * 300, 1400.f) - 1400;
		for (int k = 0; k < 2; ++k) {
			fog.setPosition(x + k * 1400, 540 + i * 55 + 8 * std::sin(t * 0.4f + i));
			fog.setFillColor(sf::Color(90, 6, 18, 22));
			w->draw(fog);
		}
	}
}

void Screens::drawMotes(sf::RenderWindow* w, float dt, sf::Color c, bool rising) {
	for (Mote& m : motes) {
		m.x += m.vx * dt + std::sin(t + m.y * 0.02f) * 6 * dt;
		m.y += (rising ? m.vy : -m.vy * 0.7f) * dt;
		if (m.y < -10) m.y = 778; if (m.y > 778) m.y = -10;
		if (m.x < -10) m.x = 1034; if (m.x > 1034) m.x = -10;
		sf::CircleShape d(m.r);
		d.setPosition(m.x, m.y);
		d.setFillColor(sf::Color(c.r, c.g, c.b, (sf::Uint8)(m.a * (0.6f + 0.4f * std::sin(t * 2 + m.x)))));
		w->draw(d);
	}
}

void Screens::drawLogo(sf::RenderWindow* w, float x, float y, float height, float alpha) {
	// Occasional flicker, like a failing bulb.
	float flick = (std::fmod(t, 7.3f) < 0.12f || std::fmod(t, 11.1f) < 0.08f) ? 0.45f : 1.f;
	sf::Uint8 a = (sf::Uint8)(255 * alpha * flick);
	if (hasLogo) {
		float k = height / logoTex.getSize().y;
		// soft blood glow behind the lettering
		logo.setScale(k * 1.01f, k * 1.01f);
		logo.setPosition(x - 2, y - 2);
		logo.setColor(sf::Color(255, 40, 60, (sf::Uint8)(a * 0.25f)));
		w->draw(logo, sf::BlendAdd);
		logo.setScale(k, k);
		logo.setPosition(x, y);
		logo.setColor(sf::Color(255, 255, 255, a));
		w->draw(logo);
	}
	else {
		const char* words[] = { "The", "Crimson", "Sleep" };
		for (int i = 0; i < 3; ++i) {
			sf::Text tx(words[i], fontTitle, (unsigned)(height / 3.2f));
			tx.setFillColor(sf::Color(CRIMSON.r, CRIMSON.g, CRIMSON.b, a));
			tx.setPosition(x + 40 * (i == 1 ? 0 : 1), y + i * height / 3.1f);
			w->draw(tx);
		}
	}
}

void Screens::centered(sf::RenderWindow* w, const sf::Font& f, const std::string& s, unsigned size, sf::Color c, float y, float cx) {
	sf::Text tx(s, f, size);
	tx.setFillColor(c);
	sf::FloatRect b = tx.getLocalBounds();
	tx.setPosition(std::floor(cx - b.width / 2 - b.left), std::floor(y));
	w->draw(tx);
}

// Keyboard (Up/Down/W/S + Enter) and mouse menu.  Returns chosen index or -1.
int Screens::menu(sf::RenderWindow* w, const char* const* items, int n, float cx, float y, float size) {
	if (key_hit(sf::Keyboard::Up) || key_hit(sf::Keyboard::W)) menuIndex = (menuIndex + n - 1) % n;
	if (key_hit(sf::Keyboard::Down) || key_hit(sf::Keyboard::S)) menuIndex = (menuIndex + 1) % n;
	if (menuIndex >= n) menuIndex = 0;
	sf::Vector2f mouse = w->mapPixelToCoords(sf::Mouse::getPosition(*w));
	int chosen = -1;
	for (int i = 0; i < n; ++i) {
		bool sel = i == menuIndex;
		std::string s = sel ? std::string("\xE2\x80\x94  ") + items[i] + "  \xE2\x80\x94" : items[i];
		sf::Text tx(sf::String::fromUtf8(s.begin(), s.end()), fontSerif, (unsigned)size);
		sf::FloatRect b = tx.getLocalBounds();
		tx.setPosition(std::floor(cx - b.width / 2 - b.left), std::floor(y + i * size * 1.25f));
		if (tx.getGlobalBounds().contains(mouse)) {
			menuIndex = i;
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) chosen = i;
		}
		float glow = sel ? 0.85f + 0.15f * std::sin(t * 3) : 1.f;
		tx.setFillColor(sel ? sf::Color((sf::Uint8)(BONE.r * glow), (sf::Uint8)(BONE.g * glow), (sf::Uint8)(BONE.b * glow)) : ASH);
		w->draw(tx);
	}
	if (key_hit(sf::Keyboard::Return) || key_hit(sf::Keyboard::Space)) chosen = menuIndex;
	return chosen;
}

// ------------------------------------------------------------------ boot
int Screens::boot(sf::RenderWindow* w, float dt) {
	t += dt; slideT += dt;
	w->setView(w->getDefaultView());
	Slide s;
	if (!slides.front(&s)) return ST_TITLE;
	float fade = slideT < 0.8f ? slideT / 0.8f : (s.seconds - slideT < 0.8f ? (s.seconds - slideT) / 0.8f : 1.f);
	if (fade < 0) fade = 0;
	if (s.logo) {
		// Logo emerges from the dark over the backdrop, then hands over to the title.
		drawBackdrop(w, sf::Color((sf::Uint8)(255 * fade), (sf::Uint8)(255 * fade), (sf::Uint8)(255 * fade)));
		drawMotes(w, dt, sf::Color(200, 40, 50), true);
		float rise = slideT / s.seconds;
		drawLogo(w, 512 - 220, 140 - 10 * rise, 420, slideT < 1.5f ? slideT / 1.5f : 1.f);
	}
	else {
		centered(w, fontItalic, s.line1, 30, sf::Color(200, 180, 172, (sf::Uint8)(255 * fade)), 340);
		if (s.line2[0]) centered(w, fontSerif, s.line2, 24, sf::Color(176, 30, 58, (sf::Uint8)(255 * fade)), 390);
	}
	if (slideT >= s.seconds || anyKey()) { slides.dequeue(); slideT = 0; }
	return slides.empty() ? ST_TITLE : ST_BOOT;
}

// ----------------------------------------------------------------- title
int Screens::title(sf::RenderWindow* w, float dt) {
	t += dt;
	drawBackdrop(w);
	// Darken the left third so the logo and menu read clearly (as in the Godot build).
	sf::VertexArray shade(sf::Quads, 4);
	shade[0] = sf::Vertex(sf::Vector2f(0, 0), sf::Color(0, 0, 0, 200));
	shade[1] = sf::Vertex(sf::Vector2f(620, 0), sf::Color(0, 0, 0, 0));
	shade[2] = sf::Vertex(sf::Vector2f(620, 768), sf::Color(0, 0, 0, 0));
	shade[3] = sf::Vertex(sf::Vector2f(0, 768), sf::Color(0, 0, 0, 200));
	w->draw(shade);
	drawMotes(w, dt, sf::Color(200, 40, 50), true);

	const float cx = 290;
	float logoH = 330;
	float logoW = hasLogo ? logoH * logoTex.getSize().x / logoTex.getSize().y : 380;
	drawLogo(w, cx - logoW / 2, 70, logoH, 1.f);
	centered(w, fontItalic, "some dreams don't let you wake up", 24, sf::Color(222, 206, 196), 424, cx);
	const char* items[] = { "Begin the dream", "Controls", "Wake up" };
	int c = menu(w, items, 3, cx, 480, 38);
	if (c == 0) return ST_NEW_GAME;
	if (c == 1) { menuIndex = 0; return ST_CONTROLS; }
	if (c == 2) w->close();
	return ST_TITLE;
}

int Screens::controls(sf::RenderWindow* w, float dt) {
	t += dt;
	drawBackdrop(w, sf::Color(110, 90, 90));
	sf::RectangleShape p(sf::Vector2f(880, 600)); p.setPosition(72, 84); p.setFillColor(sf::Color(8, 2, 4, 215));
	p.setOutlineColor(sf::Color(120, 6, 18)); p.setOutlineThickness(2); w->draw(p);
	centered(w, fontTitle, "Controls", 44, CRIMSON, 92);
	const char* keys =
		"Arrows / WASD     walk\n"
		"E / Enter              interact, open doors\n"
		"F                          flare the lantern\n"
		"B                          go back one room\n"
		"H                          ask the house for a hint\n"
		"I / J                    inventory / notes on the dead\n"
		"TAB                      data-structure panel\n"
		"Space / Esc           dismiss text / pause";
	sf::Text k(keys, fontSerif, 23); k.setFillColor(BONE); k.setPosition(110, 160); w->draw(k);
	const char* tips =
		"How to survive\n\n"
		"- The Weeping Bride hunts by sound. Freeze, or flare.\n"
		"- Never turn your back on the Hollow Child.\n"
		"- Spirits walk fixed paths: watch, then cross.\n"
		"- Fear kills too: keep the lantern lit and rest.\n"
		"- Stuck for a minute? A trail of blood appears.\n"
		"- Some doors open only to knowledge, not keys.";
	sf::Text tp(tips, fontItalic, 21); tp.setFillColor(sf::Color(220, 170, 170)); tp.setPosition(560, 160); w->draw(tp);
	centered(w, fontItalic, "Enter or Esc to return", 20, ASH, 640);
	if (key_hit(sf::Keyboard::Return) || key_hit(sf::Keyboard::Escape) || key_hit(sf::Keyboard::Space)) return -100; // caller decides
	return ST_CONTROLS;
}

int Screens::pause(sf::RenderWindow* w, float dt) {
	t += dt;
	drawBackdrop(w, sf::Color(90, 70, 70));
	drawLogo(w, 512 - 150, 80, 230, 0.8f);
	const char* items[] = { "Return to the dream", "Controls", "Leave to the title", "Wake up" };
	int c = menu(w, items, 4, 512, 380, 36);
	if (key_hit(sf::Keyboard::Escape)) return ST_PLAYING;
	if (c == 0) return ST_PLAYING;
	if (c == 1) { menuIndex = 0; return ST_CONTROLS; }
	if (c == 2) { menuIndex = 0; return ST_TITLE; }
	if (c == 3) w->close();
	return ST_PAUSED;
}

int Screens::retry(sf::RenderWindow* w, float dt, const Game& g) {
	t += dt;
	w->setView(w->getDefaultView());
	sf::RectangleShape r(sf::Vector2f(1024, 768)); r.setFillColor(sf::Color(18, 0, 3)); w->draw(r);
	drawMotes(w, dt, sf::Color(150, 10, 20), false);
	bool mind = g.deathTip == TIP_FEAR;
	centered(w, fontTitle, mind ? "Your Mind Breaks" : "You Died", 86, CRIMSON, 150);
	centered(w, fontItalic, g.deathReason, 28, BONE, 270);
	centered(w, fontSerif, std::string("Remember - ") + TIPS[g.deathTip].title + ":", 22, ASH, 330);
	std::string body = TIPS[g.deathTip].body;
	size_t nl = body.find('\n');
	std::string first = nl == std::string::npos ? body : body.substr(nl + 1);
	nl = first.find('\n'); if (nl != std::string::npos) first = first.substr(0, nl);
	centered(w, fontSerif, first, 22, sf::Color(220, 170, 170), 360);
	const char* items[] = { "Dream again", "Leave to the title" };
	int c = menu(w, items, 2, 512, 460, 36);
	if (c == 0) return ST_PLAYING;
	if (c == 1) { menuIndex = 0; return ST_TITLE; }
	return ST_RETRY;
}

// ---------------------------------------------------------------- ending
int Screens::ending(sf::RenderWindow* w, float dt, const RunStats& st) {
	t += dt; endT += dt;
	w->setView(w->getDefaultView());
	const int e = ending_type;
	struct Script { const char* lines[6]; const char* name; };
	static const Script scripts[] = {
		{ { "" }, "" },
		{ { "The corpse crumbles into grey ash.", "You stagger out into a cold, colourless morning.", "Behind you, faint and far away, a woman is still weeping.", "Somewhere beneath the chapel, something was left undone.", "" }, "ESCAPE ENDING" },
		{ { "The petrol catches. The house screams.", "Smoke fills the corridors - every door slams shut.", "Through the flames they come, all ten of them.", "The fire burns for three days.", "No one finds you. The house simply... sleeps again.", "" }, "TRAPPED ENDING" },
		{ { "You run, the locket burning in your fist.", "The front door gives way to a forest of black trees.", "The Bride walks beside you now - quiet, at peace.", "But beneath the house, the Devil still sleeps.", "And one day, he will dream of you.", "" }, "SECRET ENDING" },
		{ { "The blade falls. The Devil's corpse shrieks and turns to ash.", "Ten pale lights rise slowly from the floorboards.", "The Weeping Bride takes back her locket... and smiles.", "One by one, they drift up into the breaking dawn.", "For the first time since June 26, 1953,", "the house sleeps without dreaming." }, "TRUE ENDING" },
	};
	const Script& sc = scripts[e < 1 || e > 4 ? 1 : e];

	// Sky: the crimson moon slowly gives way to dawn on the good endings,
	// stays red for the secret one, turns to fire for the trapped one.
	float dawn = (e == END_TRUE ? 1.f : e == END_ESCAPE ? 0.45f : 0.f) * std::min(1.f, endT / 14.f);
	sf::Color tint(255, (sf::Uint8)(255 * (0.6f + 0.4f * dawn)), (sf::Uint8)(255 * (0.55f + 0.45f * dawn)));
	drawBackdrop(w, tint);
	if (dawn > 0) {
		// Dawn: deep blue overhead, warm gold at the horizon, fading over the water.
		sf::VertexArray sky(sf::Quads, 8);
		sf::Color top(40, 60, 110, (sf::Uint8)(120 * dawn)), mid(255, 190, 140, (sf::Uint8)(170 * dawn)), low(120, 70, 50, 0);
		sky[0] = sf::Vertex(sf::Vector2f(0, 0), top); sky[1] = sf::Vertex(sf::Vector2f(1024, 0), top);
		sky[2] = sf::Vertex(sf::Vector2f(1024, 560), mid); sky[3] = sf::Vertex(sf::Vector2f(0, 560), mid);
		sky[4] = sf::Vertex(sf::Vector2f(0, 560), mid); sky[5] = sf::Vertex(sf::Vector2f(1024, 560), mid);
		sky[6] = sf::Vertex(sf::Vector2f(1024, 768), low); sky[7] = sf::Vertex(sf::Vector2f(0, 768), low);
		w->draw(sky, sf::BlendAdd);
	}
	if (e == END_TRAPPED) {
		sf::RectangleShape fire(sf::Vector2f(1024, 768));
		fire.setFillColor(sf::Color(255, 90, 20, (sf::Uint8)(50 + 30 * std::sin(endT * 9) + std::rand() % 20)));
		w->draw(fire, sf::BlendAdd);
		drawMotes(w, dt, sf::Color(255, 140, 40), true);
	}
	else drawMotes(w, dt, e == END_TRUE ? sf::Color(255, 230, 200) : sf::Color(200, 40, 50), true);

	// Ten freed souls rise on the true ending.
	if (e == END_TRUE && endT > 3.5f) {
		for (Soul& s : souls) {
			s.y -= s.speed * dt;
			float x = s.x + 14 * std::sin(endT * 1.3f + s.phase);
			for (int k = 3; k >= 0; --k) {
				sf::CircleShape g(4.f + k * 4);
				g.setOrigin(g.getRadius(), g.getRadius());
				g.setPosition(x, s.y);
				g.setFillColor(sf::Color(255, 240, 230, (sf::Uint8)(k == 0 ? 230 : 40 / k)));
				w->draw(g, sf::BlendAdd);
			}
		}
	}
	sf::RectangleShape dim(sf::Vector2f(1024, 768)); dim.setFillColor(sf::Color(0, 0, 0, 90)); w->draw(dim);

	// Story lines appear one after another...
	int n = 0; while (n < 6 && sc.lines[n] && sc.lines[n][0]) ++n;
	const float per = 3.4f;
	for (int i = 0; i < n; ++i) {
		float lt = endT - 1.f - i * per;
		if (lt <= 0) continue;
		float a = std::min(1.f, lt / 1.2f);
		if (endT > 1.f + n * per + 2) a *= std::max(0.f, 1 - (endT - (3.f + n * per)) / 1.5f);
		centered(w, fontItalic, sc.lines[i], 30, sf::Color(240, 228, 214, (sf::Uint8)(255 * a)), 170 + i * 52.f);
	}
	// ...then the verdict, run statistics and credits.
	float tail = endT - (4.5f + n * per);
	if (tail > 0) {
		float a = std::min(1.f, tail / 1.5f);
		if (e == END_TRUE) centered(w, fontTitle, "The Crimson Sleep is Over", 60, sf::Color(255, 210, 190, (sf::Uint8)(255 * a)), 110);
		else drawLogo(w, 512 - 120, 70, 190, a);
		centered(w, fontSerif, sc.name, 34, sf::Color(CRIMSON.r + 40, CRIMSON.g + 20, CRIMSON.b + 20, (sf::Uint8)(255 * a)), 300);
		int mins = (int)st.time / 60, secs = (int)st.time % 60;
		char b[256];
		std::snprintf(b, sizeof b, "Time %d:%02d      Rooms %d / 10      Items %d      Spirits banished %d      Deaths %d      Peak fear %d%%",
			mins, secs, st.roomsSeen, st.itemsFound, st.banished, st.deaths, (int)st.peakFear);
		centered(w, fontSerif, b, 21, sf::Color(220, 200, 190, (sf::Uint8)(255 * a)), 360);
		const char* rank = e == END_TRUE ? (st.deaths == 0 ? "Rank: Dreamwalker - flawless" : st.deaths < 3 ? "Rank: Lightbringer" : "Rank: Survivor")
			: e == END_SECRET ? "Rank: The One Who Ran" : e == END_ESCAPE ? "Rank: Half-Awake" : "Rank: Ash";
		centered(w, fontItalic, rank, 26, sf::Color(255, 200, 170, (sf::Uint8)(255 * a)), 400);
		if (e != END_TRUE) centered(w, fontItalic, "Hint: return the Bride's locket before facing the Devil...", 20, sf::Color(200, 150, 150, (sf::Uint8)(200 * a)), 440);

		// Credits roll.
		const char* credits[] = { "THE CRIMSON SLEEP", "", "A Data Structures Laboratory project",
			"Graph - Stack - Queue - Priority Queue - Linked List - BST - BFS / DFS - Array", "",
			"Built upon 'Mansion Mystery' by Saurabh Kumar Prasad", "Music and sounds: Pixabay artists (see README)", "", "Thank you for playing.", "Sleep well." };
		float cy = 768 - (tail - 3) * 34;
		if (tail > 3) for (int i = 0; i < 10; ++i) {
			float y = cy + i * 34;
			if (y < 480 || y > 700) continue;
			float fa = std::min(1.f, std::min((y - 480) / 60, (700 - y) / 40));
			centered(w, i == 0 ? fontTitle : fontSerif, credits[i], i == 0 ? 30 : 21, sf::Color(230, 215, 200, (sf::Uint8)(255 * fa)), y);
		}
		if (tail > 2) centered(w, fontItalic, "Enter to return to the title", 18, sf::Color(170, 150, 150, (sf::Uint8)(160 + 60 * std::sin(t * 3))), 738);
		if (tail > 2 && key_hit(sf::Keyboard::Return)) { menuIndex = 0; return ST_TITLE; }
	}
	if (key_hit(sf::Keyboard::Escape)) { menuIndex = 0; return ST_TITLE; }
	return ST_ENDING;
}
