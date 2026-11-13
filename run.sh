#!/usr/bin/env bash
# The Crimson Sleep - launcher for Linux and macOS.
#   ./run.sh              build if needed (only what changed), then play
#   ./run.sh --rebuild    clean build first
# Any other arguments are passed to the game.
# SFML 3 installed somewhere custom?   SFML_DIR=/path/to/sfml3 ./run.sh
set -u
cd "$(dirname "$0")" || exit 1

echo "========================================================"
echo "                 THE CRIMSON SLEEP"
echo "========================================================"
echo

fail() { echo; echo "[ERROR] $1"; [ -n "${2:-}" ] && echo "        $2"; exit 1; }

# The SFML major version in an include folder (empty if there is none).
sfml_major() { sed -n 's/^#define SFML_VERSION_MAJOR[[:space:]]*\([0-9]*\).*/\1/p' "$1/SFML/Config.hpp" 2>/dev/null; }

OS="$(uname -s)"
if [ "$OS" = "Darwin" ]; then
	command -v make >/dev/null 2>&1 || fail "make / a C++ compiler is missing." "Install them with:  xcode-select --install"
	if [ -z "${SFML_DIR:-}" ]; then
		command -v brew >/dev/null 2>&1 || fail "Homebrew is missing (it installs SFML)." "Get it from https://brew.sh, then run:  brew install sfml"
		SFML_DIR="$(brew --prefix sfml 2>/dev/null)"
		[ -d "$SFML_DIR/include/SFML" ] || fail "SFML is not installed." "Run:  brew install sfml"
	fi
	INSTALL_HINT="Run:  brew upgrade sfml"
	JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
else
	command -v make >/dev/null 2>&1 && command -v g++ >/dev/null 2>&1 \
		|| fail "make / g++ is missing." "Ubuntu / Debian:  sudo apt-get install -y g++ make"
	INSTALL_HINT="Ubuntu / Debian:  sudo apt-get install -y libsfml-dev   (needs a release that ships SFML 3)
        Or build SFML 3 yourself and run:  SFML_DIR=/path/to/sfml3 ./run.sh"
	JOBS="$(nproc 2>/dev/null || echo 4)"
fi

# Which SFML will the build use, and is it version 3?
if [ -n "${SFML_DIR:-}" ]; then INC="$SFML_DIR/include"
elif [ -f /usr/local/include/SFML/Config.hpp ]; then INC=/usr/local/include
else INC=/usr/include; fi
MAJOR="$(sfml_major "$INC")"
[ -n "$MAJOR" ] || fail "SFML 3 is not installed (nothing found in $INC)." "$INSTALL_HINT"
[ "$MAJOR" = "3" ] || fail "Found SFML $MAJOR in $INC, but the game needs SFML 3." "$INSTALL_HINT"

MAKEARGS=(-j"$JOBS")
[ -n "${SFML_DIR:-}" ] && MAKEARGS+=(SFML_DIR="$SFML_DIR")

# A different SFML than last time (or --rebuild): start from a clean build,
# otherwise old object files would be linked against the wrong library.
if [ "${1:-}" = "--rebuild" ] || [ "$(cat build/.sfml 2>/dev/null)" != "$INC" ]; then
	[ "${1:-}" = "--rebuild" ] && shift
	make clean >/dev/null 2>&1
fi

if [ ! -x crimson_sleep ]; then echo "[INFO] Compiling the game (the first time takes a minute)..."
else echo "[INFO] Checking for changes..."; fi
make "${MAKEARGS[@]}" || fail "Build failed - see the messages above."
echo "$INC" > build/.sfml

echo "[INFO] Launching The Crimson Sleep..."
echo
exec ./crimson_sleep "$@"
