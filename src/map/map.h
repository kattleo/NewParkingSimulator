#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include "tile.h"

typedef struct
{
    int width;
    int height;
    Tile **tiles;
} Map;

bool map_load(Map *map, const char *filename);
void map_free(Map *map);

bool map_in_bounds(const Map *map, int x, int y);
bool map_is_walkable(const Map *map, int x, int y);

void map_print(const Map *map);

#endif
