#include "tile.h"

Tile tile_from_char(char c)
{
    Tile tile;
    tile.symbol = c;
    switch (c)
    {
    case '_':
        tile.type = TILE_WALL;
        break;
    case '|':
        tile.type = TILE_WALL;
        break;
    case 'P':
        tile.symbol = ' ';
        tile.type = TILE_PARKING;
        break;
    default:
        tile.symbol = ' ';
        tile.type = TILE_EMPTY;
        break;
    }
    return tile;
}