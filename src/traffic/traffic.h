#ifndef TRAFFIC_H
#define TRAFFIC_H

#include "../map/map.h"
#include "../vehicle/vehicle.h"
#include "../vehicle/vehicle_list.h"
#include "../path/path.h"

// Initialize all vehicles' routes (e.g. waypoints 1..N) and
// assign the first path segment for each.
void traffic_init_routes_waypoints(VehicleList *vehicles, Map *map);

// One simulation step:
// 1) move all vehicles along their current paths (with collisions)
// 2) for vehicles that finished a path but still have waypoints, plan the next path
void traffic_step(VehicleList *vehicles, Map *map);

#endif // TRAFFIC_H
