#pragma once
// ---------------------------------------------------------------------
// Str: a small hand-written string (instead of std::string - the project
// uses no STL).  A heap char buffer that grows by doubling, always
// '\0'-terminated so c_str() can be handed to C functions and SFML.
// ---------------------------------------------------------------------
#include <cstring>
#include <cstdio>
#include <SFML/System/String.hpp>

class Str {
	char* buf;
	int len, cap;
	void reserve(int n) {
		if (n + 1 <= cap) return;
		int c = cap * 2 > n + 1 ? cap * 2 : n + 1;
		char* nb = new char[c];
		std::memcpy(nb, buf, (size_t)len + 1);
		delete[] buf; buf = nb; cap = c;
	}
	void init(const char* s, int n) { cap = n + 1 > 16 ? n + 1 : 16; buf = new char[cap]; std::memcpy(buf, s, (size_t)n); buf[n] = 0; len = n; }
public:
	static const int npos = -1;
	Str() { init("", 0); }
	Str(const char* s) { init(s ? s : "", s ? (int)std::strlen(s) : 0); }
	Str(const char* s, int n) { init(s, n); }
	Str(const Str& o) { init(o.buf, o.len); }
	Str(Str&& o) noexcept : buf(o.buf), len(o.len), cap(o.cap) { o.init("", 0); }
	~Str() { delete[] buf; }
	Str& operator=(const Str& o) { if (this != &o) { len = 0; reserve(o.len); std::memcpy(buf, o.buf, (size_t)o.len + 1); len = o.len; } return *this; }
	Str& operator=(Str&& o) noexcept { if (this != &o) { delete[] buf; buf = o.buf; len = o.len; cap = o.cap; o.init("", 0); } return *this; }
	Str& operator=(const char* s) { return *this = Str(s); }

	Str& operator+=(const Str& o) { reserve(len + o.len); std::memcpy(buf + len, o.buf, (size_t)o.len + 1); len += o.len; return *this; }
	Str& operator+=(const char* s) { return *this += Str(s); }
	Str& operator+=(char c) { reserve(len + 1); buf[len++] = c; buf[len] = 0; return *this; }
	friend Str operator+(Str a, const Str& b) { a += b; return a; }
	friend Str operator+(Str a, const char* b) { a += b; return a; }
	friend Str operator+(const char* a, const Str& b) { Str r(a); r += b; return r; }
	friend Str operator+(Str a, char c) { a += c; return a; }

	bool operator==(const Str& o) const { return len == o.len && std::memcmp(buf, o.buf, (size_t)len) == 0; }
	bool operator==(const char* s) const { return std::strcmp(buf, s) == 0; }
	bool operator!=(const Str& o) const { return !(*this == o); }
	bool operator!=(const char* s) const { return !(*this == s); }
	char operator[](int i) const { return buf[i]; }

	const char* c_str() const { return buf; }
	const char* begin() const { return buf; }
	const char* end() const { return buf + len; }
	int size() const { return len; }
	bool empty() const { return len == 0; }
	void clear() { len = 0; buf[0] = 0; }
	int find(char c, int from = 0) const { for (int i = from; i < len; ++i) if (buf[i] == c) return i; return npos; }
	Str substr(int pos, int n = npos) const {
		if (pos > len) pos = len;
		if (n == npos || pos + n > len) n = len - pos;
		return Str(buf + pos, n);
	}
	static Str num(int v) { char t[16]; std::snprintf(t, sizeof t, "%d", v); return Str(t); }
	// Text for SFML (UTF-8 aware, so dashes and accents survive).
	operator sf::String() const { return sf::String::fromUtf8(buf, buf + len); }
};
