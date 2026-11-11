#pragma once
// Hand-written min / max / clamp (the project uses no STL <algorithm>).
template <typename T> inline T minOf(T a, T b) { return b < a ? b : a; }
template <typename T> inline T maxOf(T a, T b) { return a < b ? b : a; }
template <typename T> inline T clampTo(T v, T lo, T hi) { return v < lo ? lo : hi < v ? hi : v; }
