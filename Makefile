# The Crimson Sleep - build with:  make        run with:  make run
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter
LIBS      = -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system

SRC  = $(wildcard src/*.cpp)
OBJ  = $(SRC:src/%.cpp=build/%.o)
BIN  = crimson_sleep

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LIBS)

build/%.o: src/%.cpp src/*.h src/ds/*.h | build
	$(CXX) $(CXXFLAGS) -Isrc -c $< -o $@

build:
	mkdir -p build

run: $(BIN)
	./$(BIN)

# Data-structure unit tests (no SFML window needed).
test: build/ds_tests
	./build/ds_tests

build/ds_tests: tests/ds_tests.cpp src/ds/*.h | build
	$(CXX) $(CXXFLAGS) -Isrc tests/ds_tests.cpp -o $@

clean:
	rm -rf build $(BIN)

.PHONY: all run test clean
