#ifndef TILE_H
#define TILE_H

#include <stdbool.h>

typedef enum
{
    TILE_EMPTY,
    TILE_WALL,
    TILE_PARKING,
    TILE_PARKING_INDICATOR
} TileType;

struct ParkingSpot; // forward declaration (NO typedef!)

typedef struct Tile
{
    char symbol;   // visual character on the map
    TileType type; // type of tile (wall, parking, etc.)

    bool is_waypoint;
    int waypoint_id; // -1 if none

    struct ParkingSpot *spot; // pointer to parking spot
} Tile;

Tile tile_from_char(char c);

#endif