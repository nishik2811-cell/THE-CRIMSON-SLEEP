# The Crimson Sleep

*A 2D pixelated horror escape, written in C++17 with SFML 3. Every core mechanic runs on a hand-written data structure.*

> some dreams don't let you wake up

You wake on the floor of a mansion that has been "sleeping in crimson" since **June 26, 1953**, the night ten people were murdered there. Find the keys, read the clues and survive the dead. Then decide how the nightmare ends.

The game extends **[Mansion Mystery](https://github.com/saurabh1907/2d-rpg-game-cpp)** by Saurabh Kumar Prasad, an SFML 2D RPG horror game. It keeps that game's tile maps, characters and puzzles, and adds a crimson re-theme, a boot page and title screen, new ghosts, a health and fear system, a hint system, a mini-map, two new rooms, four endings, and the data-structure core described in the project synopsis.

---

## Build & run

Requires **SFML 3** (Graphics, Window, System, Audio).

### Linux (Ubuntu / Debian)
```bash
sudo apt-get install -y g++ make libsfml-dev   # must be SFML 3.x
make            # builds ./crimson_sleep
# SFML 3 installed somewhere custom?  make SFML_DIR=/path/to/sfml3
make run        # run it (must be started from this folder: it loads ./assets)
make test       # unit tests for every data structure (no window needed)
```

### Windows
* **MSYS2 / MinGW:** `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-sfml make`, then `make` in the MSYS2 MinGW shell.
* **Visual Studio + CMake:** install SFML 3, then:
  ```powershell
  cmake -B build -DSFML_DIR="C:/SFML/lib/cmake/SFML"
  cmake --build build --config Release
  ```
  CMake copies `assets/` next to the executable.

### macOS
`brew install sfml`, then `make SFML_DIR=$(brew --prefix sfml)` (point `CXXFLAGS`/`LIBS` at Homebrew's include and lib dirs if needed).

---

## Controls

| Key | Action |
|---|---|
| Arrows / WASD | Walk |
| E / Enter | Interact: pick up, open, read, use doors |
| **F** | Flare the lantern: banishes or stuns ghosts (10 s cooldown) |
| **B** | Go back one room (Stack) |
| **H** | Ask for a hint now (BFS) |
| I | Inventory (Linked List) |
| J | Notes on the dead: ghost tips |
| **TAB** | Live data-structure panel (for the viva / demo) |
| Space | Dismiss story text |
| Esc | Pause |

---

## Data structures: where each one lives and what breaks without it

Every structure is written from scratch in `src/ds/`. No `std::vector`, `std::map`, `std::stack`, `std::queue`, `std::priority_queue` or `std::list` is used in the game logic.

| Structure | File | Gameplay role |
|---|---|---|
| **Graph (adjacency list)** | `ds/Graph.h`, `world.cpp` | The house. Rooms are nodes and doors are edges. Each edge carries its door number, its lock, the item that unlocks it, and a *hidden* flag for secret passages. Every door you open goes through `Game::tryDoor`, which checks the graph. |
| **Stack** | `ds/Stack.h`, `Player::history` | Movement history. Entering a room pushes it. **B (Go Back)** pops the current room and walks you through the door to the previous one. |
| **Queue** | `ds/Queue.h`, `Game::events` | The haunted-event timeline: flickering lights, whispers, door slams, cold breath, blood writing and apparitions. They are scheduled when you enter a room and fire strictly first-in, first-out. Queues also power the BFS frontiers and the boot-screen slide sequence. |
| **Priority Queue (max-heap)** | `ds/PriorityQueue.h`, `Game::updateThreats` | Active threats, ranked by danger: Bride attack 100 > Devil 90 > Hollow Child 80 > Bride near 75 > spirit 60 > darkness 20 > calm 0. The top threat drives the on-screen warning, the fear rate and the jump scare. |
| **Linked List** | `ds/LinkedList.h`, `Player::inventory` | Inventory. Items are inserted on pickup and deleted when used: a key is removed from the list when it opens its door. |
| **Binary Search Tree** | `ds/BST.h`, `Game::itemDB` | Item and clue database, keyed by item id, with *collected* and *discovered* flags. Puzzle gates query it: the Ossuary door opens only if the 1953 newspaper record is marked *read*. |
| **BFS** | `Graph::bfsPath`, `Game::computeTrail` | (1) The room-level shortest route shown in hints. (2) A tile-level BFS over the room grid that draws a **trail of blood** from you to the objective. |
| **DFS** | `Graph::dfsExplore` | Reading the Hymn Page in the Chapel runs a DFS over *all* edges, hidden ones included. The DFS "senses" the secret Hidden Tunnel and reveals it. |
| **Array** | `world.cpp` (`ROOMS[]`, `ITEMS[]`), `ds/TileGrid.h`, `Game::maps[]` | Static room properties (name, map, danger level, darkness), baseline item metadata, and each room's fixed 32×24 tile grid (O(1) lookup). |

### Complexity

| Operation | Structure | Cost |
|---|---|---|
| BFS / DFS | Graph | O(V + E) |
| Tile trail BFS | 32×24 grid | O(W·H) |
| push / pop / top | Stack | O(1) |
| enqueue / dequeue | Queue | O(1) |
| insert / extractMax | Priority Queue | O(log n) |
| insert front · find / remove | Linked List | O(1) · O(n) |
| search / insert | BST | O(log n) average, O(n) worst |
| index | Array | O(1) |

`make test` runs `tests/ds_tests.cpp`, which checks each structure: LIFO/FIFO order, heap ordering with late high-priority inserts, list deletion, sorted BST in-order traversal, BFS detouring around a locked door, and DFS finding a hidden room.

---

## The house (10 rooms)

Entrance Hall → Grand Corridor → Library → Collapsed Bedroom / Nursery of Whispers → Chapel of Ash → Ritual Antechamber → **The Crimson Crypt**, plus the secret **Hidden Tunnel** and **The Ossuary** below the chapel. The mini-map at the top of the screen draws this graph. Unvisited rooms show as `?`, locked doors appear in dark red, and the BFS hint route lights up.

## Horrors, and how to beat them

| Horror | Behaviour | How to survive |
|---|---|---|
| **Restless Spirits** | Walk fixed patrol paths | Watch the pattern, then cross. A flare freezes them. |
| **The Weeping Bride** | Drifts *through walls* toward you, hunting by sound | **Stand still** and she loses you. **Flare** to banish her. Return her **Crimson Locket** and she never harms you again. |
| **The Hollow Child** | Crawls faster than you can run, but only when unseen | **Face it** and it freezes. Back toward the door. A flare drives it away. |
| **The Devil** | Hunts you through the crypt; EMF beeps when he's near | Dig the cracked ground with a mattock (stand still), then strike the corpse with the Blessed Sword. A flare stuns him. |
| **The house** | Spikes, fire, open graves; blood on the walls; apparitions at the edge of the light | Time your steps. Levers silence the spikes. |
| **Fear** | Rises in the dark, near the dead, and with every scare. At 100 your mind breaks. | Keep the lantern lit, rest in quiet rooms, and flare to steady your nerves. |

The first time you meet each horror, a **survival note** explains it. **J** reopens all the notes.

### Hints when you're stuck
If you go **30 seconds** without progress, the hint bar names your next objective and the BFS route to it. Once you're in the right room, a trail of blood leads you to it. After **1 minute**, the objective itself starts to pulse. Press **H** for a hint at any time.

## Endings

| Ending | How |
|---|---|
| **True** | Find the Hidden Tunnel, return the Bride's locket, *then* destroy the Devil's corpse. The crimson moon fades to dawn, ten souls rise, and the house finally sleeps without dreaming. |
| **Escape** | Destroy the corpse without the locket. |
| **Secret** | Walk out of the front door carrying the locket. |
| **Trapped** | Choose the petrol in the Antechamber. |

Each ending closes with your run stats (time, rooms, items, spirits banished, deaths, peak fear), a rank and the credits.

---

## Project layout

```
src/            game code (SFML layer + logic)
src/ds/         hand-written data structures (header-only, no SFML)
tests/          data-structure unit tests
assets/maps/    room tile maps (map9/map10 generated by tools/make_maps.py)
assets/graphics/  crimson-themed sprites, logo, backdrop, jump scare
assets/audio/   music and sound effects
assets/fonts/   Grenze Gotisch, Cormorant Garamond (SIL OFL), DejaVu Sans
tools/          Python scripts that regenerate the art, maps and backdrop
```

### Regenerating assets (Python 3 + Pillow + NumPy)
```bash
python3 tools/make_art.py          # crimson tileset/floor + pixel-art ghosts
python3 tools/make_dead_alfred.py  # Alfred after the house takes him
python3 tools/adjust_hero.py       # hero recolour (blue coat, brown hair) from the original sprite
python3 tools/make_maps.py         # Hidden Tunnel + Ossuary maps
python3 tools/make_backdrop.py     # fallback blood-moon title backdrop
python3 tools/make_logo.py logo.png              # title logo -> transparent crimson
python3 tools/make_jumpscare_from.py art.png     # your art -> crimson jump scare
```
To use your own title backdrop, drop it in as `assets/graphics/title_bg.jpg` (or `.png`). It takes priority over the generated one.

### Headless screenshots (for docs and CI)
```bash
CS_STATE=title CS_FRAMES=60 CS_SHOT=title.png ./crimson_sleep
CS_ROOM=3 CS_HINT=1 CS_FRAMES=40 CS_SHOT=hint.png ./crimson_sleep
```
`CS_ROOM=n` starts in room *n*. Also available: `CS_BRIDE=1`, `CS_JUMP=1`, `CS_OPEN=1` (data-structure panel), and `CS_STATE=ending:4`.

---

## Credits

* Base game, tiles and characters: *Mansion Mystery* by Saurabh Kumar Prasad.
* Music (chosen by the team): *Title* (title screen), *Dream* (exploring), *Creep* (dangerous rooms), *Chase* (when something is hunting you), *Wrong* (death screen and the trapped ending), *Apartments* (endings). `crack.wav` is used for cracking floorboards and digging.
* Sound effects from Pixabay: *scream* and *scary breath* (freesound_community), *scary loud screaming* (judgecruz1 / remusic), *scary bells* (Dragon Studio), *scary sound effect* (hgoliya08). The heartbeat, whisper and door slam are synthesised in-house.
* Fonts: Grenze Gotisch and Cormorant Garamond (SIL Open Font License, `assets/fonts/OFL.txt`), DejaVu Sans.
