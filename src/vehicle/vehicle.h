#ifndef VEHICLE_H
#define VEHICLE_H

#include "../map/map.h"
#include "../common/direction.h"
#include "../path/path.h"

#define MAX_ROUTE_WAYPOINTS 16

struct ParkingSpot;

typedef enum
{
    VEH_DRIVING,
    VEH_PARKING,
    VEH_PARKED,
    VEH_LEAVING,
    VEH_EXIT_QUEUE
} VehicleState;

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

typedef struct Vehicle
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

    VehicleState state;

    struct ParkingSpot *assigned_spot;
    bool wants_parking;

    int parking_spot_id;  // -1 = none
    int going_to_parking; // bool-ish
    int parking_time; // ms parked (reset when not parked)
    int reverse_steps_remaining; // for backing out
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