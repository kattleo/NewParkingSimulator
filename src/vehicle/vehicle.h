#ifndef VEHICLE_H
#define VEHICLE_H

#include "../map/map.h"
#include "../path/path.h"

#define MAX_ROUTE_WAYPOINTS 16

typedef enum
{
    DIR_NORTH,
    DIR_EAST,
    DIR_SOUTH,
    DIR_WEST
} Direction;

typedef struct
{
    int width;
    int height;
    char **rows; // rows[height][width], ' ' = transparent
} Sprite;

// A set of sprites for all 4 directions
typedef struct
{
    Sprite north;
    Sprite east;
    Sprite south;
    Sprite west;
} VehicleSprites;

typedef struct
{
    int x;
    int y;
    Direction dir;
    const VehicleSprites *sprites; // pointer to shared sprites

    // Car follows route across waypoints
    // It reaches every waypoint with a path
    Path path;
    int path_index; // index of next step in path
    int has_path;

    int route[MAX_ROUTE_WAYPOINTS]; // sequence of waypoint IDs
    int route_length;
    int route_pos; // index into route[]
} Vehicle;

// Initialize global/default vehicle sprites from 4 txt files
// base_path like "assets/sprites"
bool vehicle_sprites_init(const char *base_path);
const VehicleSprites *vehicle_sprites_get_default(void);

// Initialize a vehicle at (x, y) with direction and glyph (e.g. 'C')
void vehicle_init(Vehicle *v, int x, int y, Direction dir);

// Get Sprite according to vehicle direction
const Sprite *vehicle_get_sprite(const Vehicle *v);

void vehicle_set_path(Vehicle *v, const Path *p);

#endif