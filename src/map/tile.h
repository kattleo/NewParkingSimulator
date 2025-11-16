#ifndef TILE_H
#define TILE_H

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
} Tile;

Tile tile_from_char(char c);

#endif
