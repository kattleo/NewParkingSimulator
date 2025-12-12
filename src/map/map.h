#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include "tile.h"
#include "waypoint.h"
#include "../common/direction.h"

struct Vehicle; // forward declaration
typedef struct Vehicle Vehicle;



// Only one gate supported (vertical run of 'G')
#define MAX_GATE_TILES 32
typedef struct {
    int xs[MAX_GATE_TILES];
    int ys[MAX_GATE_TILES];
    int tile_count;
    int open; // 1=open, 0=closed
} Gate;

// Forward declare Map for API
struct Map;
typedef struct Map Map;

// Gate control API (only one gate)
void map_set_gate_open(Map *map, int open);
int map_get_gate_open(const Map *map);

#define MAX_WAYPOINTS 32
#define MAX_PARKING_SPOTS 64

typedef struct ParkingSpot
{
    int id;
    int x0, y0; // anchor (upper-left of 2x6 block)
    int width;
    int height;
    int indicator_x;
    int indicator_y;
    int capacity; // number of tiles
    int occupied;
    Vehicle *occupant;
} ParkingSpot;

typedef struct Map
{
    int width;
    int height;
    Tile **tiles;
    ParkingSpot parkings[128];
    int parking_count;
    // Entry and exit gates
    Gate gate_entry; // 'G'
    Gate gate_exit;  // 'g'
    // waypoint fields...
    Waypoint waypoints[16];
    int waypoint_count;
    // Start position (set by 'S' in map)
    int start_x;
    int start_y;
    int has_start;
    // End position (set by 'E' in map)
    int end_x;
    int end_y;
    int has_end;
} Map;

bool map_load(Map *map, const char *filename);
void map_free(Map *map);

bool map_in_bounds(const Map *map, int x, int y);
bool map_is_walkable(const Map *map, int x, int y);

void map_print(const Map *map);

const Waypoint *map_get_waypoint_by_id(const Map *map, int id);
#include "waypoint.h"

void map_build_parking_spots(Map *map);

const ParkingSpot *map_get_parking_spot_with_indicator(const Map *map, int x, int y);

#endif
