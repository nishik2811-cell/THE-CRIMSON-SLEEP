# The Crimson Sleep

**A 2D pixelated horror escape game built in C++ and SFML for a Data Structures Laboratory project.**

This is **not** a tech demo with data structures bolted on for show — every required data
structure is load-bearing gameplay logic. Delete any one of them and a real feature breaks:
remove the Stack and "Go Back" stops working, remove the BST and the Basement Key puzzle can be
picked up without ever reading the Diary Page, remove the max-heap Priority Queue and a Ghost
Attack could resolve *after* a harmless Dark Room event instead of before it.

---

## Table of Contents

- [Story & Gameplay](#story--gameplay)
- [Features](#features)
- [Data Structures Used](#data-structures-used--why)
- [Algorithms](#algorithms)
- [Controls](#controls)
- [Technology Stack](#technology-stack)
- [Project Structure](#project-structure)
- [How to Compile and Run](#how-to-compile-and-run)
- [Example Gameplay Flow](#example-gameplay-flow)
- [Complexity Table](#complexity-table)
- [Data Structures Debug Panel](#data-structures-debug-panel-for-the-viva)
- [Team](#team)

---

## Story & Gameplay

You wake up alone in the Entrance Hall of a haunted house with no memory of how you got there.
The front door will not budge. Somewhere in the house's 13 interconnected rooms are the keys,
clues, and courage you'll need to either escape, uncover the truth, or become part of the house
forever.

- Explore **13 rooms**: Entrance Hall, Library, Living Room, Kitchen, Bedroom, Bathroom, Study,
  Basement, Attic, Garden, Hidden Tunnel, Ritual Room, Secret Room.
- Collect **7 items/clues**: Rusty Key, Basement Key, Torch, Old Photograph, Diary Page, Ritual
  Symbol, Strange Coin.
- Solve a linear but interconnected puzzle chain to unlock doors and progress.
- Manage **Health** (0-100) and **Fear** (0-100). Health hits 0 → you die. Fear hits 100 → you
  lose your mind. Either way, it's game over.
- Face randomly-triggered **haunted events** (lights flicker, whispers, footsteps...) and
  **threats** (dark rooms, traps, ghosts) whose severity is resolved most-dangerous-first.
- Reach one of **four different endings** depending on your health, fear, discovered clues, and
  the route you choose to leave by.

---

## Features

- Full keyboard-driven gameplay loop: movement, interaction, inventory, hints, pausing.
- Procedurally generated horror sound effects (ambient drone, jumpscare stinger, whispers,
  heartbeat, door creaks) — no downloaded audio assets required.
- A jumpscare screen effect (flash + screen shake + stinger sound) tied directly to the
  Priority Queue resolving a Ghost Attack threat.
- A live **Data Structures Debug Panel** (press `TAB`) that shows the real internal state of
  every required data structure while you play — built for demonstrating the project in a viva.
- Four distinct, meaningfully different endings.
- Clean separation between game logic (`Game`, `Player`, and the data structure classes — zero
  SFML dependency) and the SFML rendering/input layer (`main.cpp`).

---

## Data Structures Used — Why

Every data structure below is **manually implemented** (no `std::vector`, `std::map`,
`std::set`, `std::priority_queue`, or `std::stack`/`std::queue` anywhere in the core logic).

| Data Structure | File(s) | Real gameplay role |
|---|---|---|
| **Graph (Adjacency List)** | `Graph.h` / `Graph.cpp` | Represents the entire house. Each room is a node; each doorway is an edge stored in a linked list hanging off `adjacencyHead[roomId]`. Some edges start **locked** (e.g. Kitchen → Basement). All player movement (`Game::confirmMove`) walks this structure — you physically cannot move to a room that isn't connected and unlocked. |
| **Stack (linked list)** | `Stack.h` / `Stack.cpp` | Stores the player's room-visit history. Every room entered is **pushed**. Pressing `B` (Go Back) **pops** the current room and returns you to the previous one — a real, working undo-movement feature, not a log. |
| **Queue (linked list)** | `Queue.h` / `Queue.cpp` | Holds haunted environmental events (lights flicker, door slams, whispers...) in the exact order they occur. `Game::update()` **dequeues** and resolves the oldest pending event every few seconds — strict FIFO. |
| **Priority Queue (array-based max-heap)** | `PriorityQueue.h` / `PriorityQueue.cpp` | Holds active threats (Dark Room, Trap, Ghost Nearby, Ghost Attack), each with a numeric danger priority. `extractMax()` always resolves the single most dangerous threat first — even if a Ghost Attack was queued *after* a harmless Dark Room event, the attack still resolves first. |
| **Linked List (singly linked)** | `LinkedList.h` / `LinkedList.cpp` | The player's inventory. Items are added to the front on pickup and removed by name when consumed (e.g. using the Rusty Key on a locked door actually deletes it from the list). |
| **Binary Search Tree** | `BST.h` / `BST.cpp` | A searchable database of every item/clue, keyed by id. Used for real puzzle gates: picking up the Basement Key checks `clueDB.search(DIARY_PAGE_ID)->discovered` first — if you haven't read the Diary Page, the key means nothing to you and you can't take it. |
| **Array** | `Room.h` / `Room.cpp`, `Game.h` (`itemMeta`, `roomItemId`) | Static, fixed-size metadata: room names/descriptions/danger levels, and item metadata. This data never grows/shrinks at runtime, so a plain C-style array is the honest choice — not a linked structure. |

Because `Stack`, `Queue`, `LinkedList`, `BST`, and `Graph` all own their nodes through raw
pointers, their copy constructor and copy-assignment operator are explicitly `= delete`d
(Rule of Three) so the compiler catches accidental shallow copies at compile time instead of
causing a double-free at runtime.

---

## Algorithms

- **Breadth-First Search (BFS)** — `Graph::bfsShortestPath()`. Powers the in-game hint system
  (`H` key): finds the shortest currently-passable route from your room to whatever your next
  objective is (get diary, unlock basement, find the ritual symbol, etc.) and tells you the next
  room to walk into.
- **Depth-First Search (DFS)** — `Graph::dfsExplore()` / `dfsCanReach()`. Powers the "search the
  Basement" feature (`F` key): explores every room reachable from the Basement — including
  currently locked doors, since this represents *knowledge* of the layout, not physical movement
  — and reveals the Hidden Tunnel passage as a result. Also used to check secret-area
  reachability for the debug panel.

Both are implemented from scratch (manual visited arrays, manual parent-pointer path
reconstruction for BFS, a small fixed-size circular queue local to `Graph.cpp`, and recursive
DFS) — no `<algorithm>` graph utilities.

---

## Controls

| Key | Action |
|---|---|
| `↑ / W` or `← / A` | Select previous door |
| `↓ / S` or `→ / D` | Select next door |
| `Enter` / `Space` | Move through the selected door (Graph movement) |
| `B` | **Go Back** to the previous room (Stack) |
| `E` | Interact — pick up items, unlock doors, use an exit |
| `F` | Inspect a held clue, or search the room for secrets (BST + DFS) |
| `H` | Request a hint toward your next goal (BFS) |
| `I` | Toggle Inventory panel (Linked List) |
| `TAB` | Toggle the Data Structures debug panel |
| `ESC` | Pause / Resume |

Controls are also shown in-game (Main Menu → `C`) and in a bottom strip during play.

---

## Technology Stack

- **Language:** C++17
- **Graphics/Audio:** [SFML 3](https://www.sfml-dev.org/) (Graphics, Window, System, Audio modules)
- **Build system:** CMake ≥ 3.16
- **Compiler:** GCC/G++ (tested with GCC 15) or MSVC on Windows

---

## Project Structure

```
The Crimson Sleep/
├── include/                 # Headers
│   ├── Graph.h               # Adjacency-list graph (house layout)
│   ├── Stack.h                # Linked-list stack (movement history)
│   ├── Queue.h                 # Linked-list queue (haunted events)
│   ├── Event.h                  # Event type + factory
│   ├── PriorityQueue.h           # Array-based max-heap (threats)
│   ├── Threat.h                    # Threat type + factory
│   ├── LinkedList.h                 # Singly linked list (inventory)
│   ├── Item.h                        # Item POD struct
│   ├── BST.h                          # Binary search tree (clue database)
│   ├── Room.h                          # Room ids + static room metadata array
│   ├── Player.h                         # Player state (owns Stack + LinkedList)
│   ├── Game.h                            # Core game logic (owns everything above)
│   └── AudioManager.h                     # Procedural SFML sound effects
├── src/                      # Implementations
│   ├── Graph.cpp, Stack.cpp, Queue.cpp, Event.cpp, PriorityQueue.cpp,
│   │   Threat.cpp, LinkedList.cpp, BST.cpp, Room.cpp, Player.cpp, Game.cpp
│   ├── AudioManager.cpp
│   └── main.cpp              # SFML rendering + input layer only (no game logic)
├── assets/
│   └── fonts/DejaVuSans.ttf  # Bundled font (no external downloads needed)
├── CMakeLists.txt
├── .gitignore
└── README.md
```

`Game`/`Player` and every data-structure class have **zero SFML dependency** — they were
developed and unit-tested as plain console programs before any graphics were added (Development
Approach, stages 1–5), which is why the logic can be explained and verified independently of the
rendering code.

---

## How to Compile and Run

### Dependencies

- CMake ≥ 3.16
- A C++17 compiler
- SFML 3 (Graphics, Window, System, Audio components)

### Linux / Ubuntu

```bash
sudo apt-get update
sudo apt-get install -y cmake libsfml-dev g++

cd "The Crimson Sleep"
mkdir -p build && cd build
cmake ..
make
./TheCrimsonSleep
```

The build step automatically copies `assets/` next to the built executable, so it can be run
from anywhere.

### Windows

1. Install [CMake](https://cmake.org/download/) and a compiler (Visual Studio 2019+ with the
   "Desktop development with C++" workload, or MinGW-w64).
2. Install SFML 3 for your compiler from the [SFML downloads page](https://www.sfml-dev.org/download.php),
   or build it from source.
3. Configure with CMake, pointing it at your SFML install if it isn't auto-detected:
   ```powershell
   mkdir build
   cd build
   cmake .. -DSFML_DIR="C:/path/to/SFML/lib/cmake/SFML"
   cmake --build . --config Release
   ```
4. Run the produced `TheCrimsonSleep.exe` (copy `assets/` next to it if your generator doesn't
   run the post-build copy step).

---

## Example Gameplay Flow

1. Start in the **Entrance Hall**. Doors to Library, Living Room, and Garden are open.
2. Go to the **Library**, pick up the **Diary Page** (`E`), then read it (`F`) — this sets a
   flag in the BST that later gates the Basement Key.
3. Go to the **Living Room**, pick up the **Rusty Key**.
4. Back in the Library, `E` unlocks the **Study** door using the Rusty Key.
5. In the **Study**, `E` picks up the **Basement Key** — this only works because the Diary Page
   was already read.
6. Reach the **Kitchen** (via Living Room), pick up the **Torch**, then `E` unlocks the
   **Basement**.
7. In the **Basement**, pick up the **Ritual Symbol**, then `F` to search — this runs a DFS from
   the Basement and reveals the **Hidden Tunnel**.
8. In the Hidden Tunnel, `E` unlocks the way to the **Ritual Room** using the Ritual Symbol.
9. Separately, visit the **Attic** for the **Old Photograph** (read it with `F`) and the
   **Garden** for the **Strange Coin**.
10. Back in the Ritual Room, `E` unlocks the **Secret Room** (needs the coin *and* having read
    the photograph).
11. Entering the Secret Room reveals the house's hidden truth and unlocks a shortcut back to the
    Garden.
12. **Choose your ending:** interact with the front door in the Entrance Hall, or take the
    secret shortcut out through the Garden.

Throughout all of this, haunted events and threats fire semi-randomly based on each room's
danger level — press `TAB` at any time to see exactly what the Queue and Priority Queue are
currently holding.

---

## Complexity Table

| Operation | Structure | Complexity |
|---|---|---|
| BFS / DFS traversal | Graph | O(V + E) |
| Push / Pop | Stack | O(1) |
| Enqueue / Dequeue | Queue | O(1) |
| Insert / Extract-Max | Priority Queue (heap) | O(log n) |
| Insert at front | Linked List | O(1) |
| Search / Remove by name | Linked List | O(n) |
| Search / Insert (average) | Binary Search Tree | O(log n) |
| Search / Insert (worst case) | Binary Search Tree | O(n) |
| Access by index | Array | O(1) |

---

## Data Structures Debug Panel (for the viva)

Press `TAB` during gameplay to open a live panel showing the **actual current contents** of
every required data structure, updated in real time:

- **Graph** — the current room's neighbors and their lock state.
- **Stack** — the full movement history, top to bottom.
- **Queue** — every haunted event currently waiting to be processed, in order.
- **Priority Queue** — every active threat and its numeric priority.
- **Linked List** — the current inventory.
- **BST** — every clue/item in the database, in sorted (in-order) order, with its
  discovered/unread state.
- **BFS** — the most recently computed hint path.
- **DFS** — the most recent room-exploration result.

This is intended to be opened during a live demonstration so the examiner can see the data
structures being read and mutated as you play, not just trust that they exist somewhere in the
code.


