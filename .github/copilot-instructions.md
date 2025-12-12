# Copilot instructions — NewParkingSimulator

Quick start
- Build: `make` (Makefile uses `gcc -std=c99 -Wall -Wextra -O2`).
- Run from project root: `./main` (binary expects assets in `assets/`, e.g. `assets/map.txt` and `assets/carSmall/*`).

Execute `make` whenever applying changes to the source files. Fix any errors that occur automatically.

High-level architecture (what to know first)
- `src/map/` — ASCII map loader + parking detection. `assets/map.txt` encodes walls (`_`, `|`), park blocks `P`, and waypoint digits `1`..`9`.
- `src/path/` — BFS pathfinder (4-way, no diagonals). Constraint: `MAX_PATH_STEPS = 1024`.
- `src/vehicle/` — `Vehicle` model, sprite loader (`vehicle_sprites_init`) and a linked `VehicleList` which stores copies of `Vehicle` structs (important for ownership semantics).
- `src/traffic/` — high-level traffic behavior: assigns routes, finds nearby parking spots, flags `ParkingSpot->occupied` and calls `vehicles_update_all` to apply motion & collision rules.
- `src/render/` — terminal renderer, uses ANSI colors and Unicode box-drawing characters. Parking indicators are displayed based on tile type `TILE_PARKING_INDICATOR` and `ParkingSpot->occupied`.
- `src/main.c` — simple simulation loop: spawn interval, draw background and paths, draw vehicles, step traffic.

Key conventions & constraints
- Code targets C99 and is written to be small and local (no external libs). Keep changes simple and explicit.
- Sizes and limits are defined as macros: `MAX_WAYPOINTS`, `MAX_PARKING_SPOTS`, `MAX_ROUTE_WAYPOINTS`, `MAX_PATH_STEPS` — check these before changing algorithms.
- Sprites are plain text files with fixed width/height. `vehicle.c` contains hard-coded sizes for car variants (update these constants if you add a new sprite type).
- `vehicle_list_push_back` copies the `Vehicle` value into the node; returned pointer is to the internal copy. Avoid storing pointers to function-local `Vehicle` values after the call.
- Map loader allocates `Tile** tiles` in `map_load` and expects `map_free` to be called to avoid leaks.

Common tasks & example pointers
- To add a waypoint: edit `assets/map.txt` and add a digit `1`..`9` at the desired location (map loader converts digits to waypoints).
- To change spawn position: modify coordinates in `src/main.c` where `vehicle_init(&v, 133, 27, DIR_EAST)` is used.
- To change parking detection: inspect `map_build_parking_spots` in `src/map/map.c` (it looks for 2×6 `P` blocks and marks indicator tiles).
- To debug pathfinding: call `path_find` directly or print paths; BFS is in `src/path/path.c` (no heuristics, deterministic shortest path on 4 neighbors).
- Rendering notes: `screen_present` maps ASCII symbols to box-drawing glyphs; it also expects `Tile::spot` to be set for indicators (if you add new parking logic, ensure the tile->spot pointer is maintained).

Tests / debug workflow
- There are no automated tests in the repo. Typical dev flow: `make` → `./main` → observe terminal UI. Use printf debug lines (e.g., `traffic.c` prints when a car parks).
- Use `valgrind ./main` or `gcc` sanitizer builds locally for memory checks if you change allocations.

PR / change suggestions for AI agents
- Keep changes small and local; prefer adding new files under `src/<subdir>/` so Makefile picks them up.
- When adding sprites, ensure file width/height match loader expectations; update constants in `src/vehicle/vehicle.c` if needed.
- If you add new tile types that alter rendering state, update both `src/map/tile.h` and `src/render/render.c` (render logic switches on tile symbol/type and uses `tile->spot` for parking indicator state).

Files to inspect first (quick links)
- `src/main.c`, `src/map/map.c`, `src/path/path.c`, `src/vehicle/vehicle.c`, `src/vehicle/vehicle_list.c`, `src/traffic/traffic.c`, `src/render/render.c`.

General Instructions for AI Agents:
- Whenever you are unsure about how to proceed, refer back to this document for context on architecture, conventions, and common tasks. If you dont find the answer here, ask for clarification.


Game Description:
### Game Goals
- Vehicles spawn and find dynamic waypoint routes

- Vehicles follow paths smoothly with collision avoidance

- Vehicles detect nearby free parking spots

- Once a parking spot is chosen:

- A new path is computed immediately

- Vehicle transitions into parking mode

- Avoid wall collisions using:

  - Vehicle radius

  - Orientation-based collision shrink


  ### Traffic Routing (Waypoints)

Waypoints are static large-scale navigation nodes used to reduce expensive pathfinding calls.

Cars:

Get a waypoint sequence from spawn → exit

Follow each with pathfinding per segment

When near a parking spot, waypoint flow is overridden