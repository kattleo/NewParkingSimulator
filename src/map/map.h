#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include "tile.h"
#include "../common/direction.h"

struct Vehicle; // forward declaration
typedef struct Vehicle Vehicle;

typedef struct
{
    int x;
    int y;
    int id;
} Waypoint;

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

    // waypoint fields...
    Waypoint waypoints[16];
    int waypoint_count;

} Map;

bool map_load(Map *map, const char *filename);
void map_free(Map *map);

bool map_in_bounds(const Map *map, int x, int y);
bool map_is_walkable(const Map *map, int x, int y);

void map_print(const Map *map);

const Waypoint *map_get_waypoint_by_id(const Map *map, int id);

void map_build_parking_spots(Map *map);

const ParkingSpot *map_get_parking_spot_with_indicator(const Map *map, int x, int y);

#endif
