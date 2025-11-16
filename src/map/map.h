#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include "tile.h"

typedef struct
{
    int x;
    int y;
    int id;
} Waypoint;

#define MAX_WAYPOINTS 32

typedef struct
{
    int width;
    int height;
    Tile **tiles;

    Waypoint waypoints[MAX_WAYPOINTS];
    int waypoint_count;
} Map;

bool map_load(Map *map, const char *filename);
void map_free(Map *map);

bool map_in_bounds(const Map *map, int x, int y);
bool map_is_walkable(const Map *map, int x, int y);

void map_print(const Map *map);

const Waypoint *map_get_waypoint_by_id(const Map *map, int id);

#endif
