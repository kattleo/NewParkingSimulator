#include "tile.h"

Tile tile_from_char(char c)
{
    Tile tile;
    tile.symbol = c;

    if (c >= '1' && c <= '9')
    {
        tile.type = TILE_EMPTY;
        tile.symbol = ' ';
    }
    else if (c == '_' || c == '|')
    {
        tile.type = TILE_WALL;
    }
    else if (c == 'P')
    {
        tile.symbol = ' ';
        tile.type = TILE_PARKING;
    }
    else
    {
        tile.symbol = ' ';
        tile.type = TILE_EMPTY;
    }

    return tile;
}