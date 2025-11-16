#ifndef TILE_H
#define TILE_H

#include <stdbool.h>

typedef enum
{
    TILE_EMPTY,
    TILE_WALL,
    TILE_PARKING
} TileType;

typedef struct
{
    char symbol;   // visual character on the map
    TileType type; // type of tile (wall, parking, etc.)

    bool is_waypoint;
    int waypoint_id; // -1 if none
} Tile;

Tile tile_from_char(char c);

#endif