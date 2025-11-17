#ifndef TRAFFIC_H
#define TRAFFIC_H

#include "../map/map.h"
#include "../vehicle/vehicle.h"
#include "../vehicle/vehicle_list.h"
#include "../path/path.h"

void traffic_init_vehicle_route(Vehicle *v, Map *map);

// One simulation step:
// 1) move all vehicles along their current paths (with collisions)
// 2) for vehicles that finished a path but still have waypoints, plan the next path
void traffic_step(VehicleList *vehicles, Map *map);

#endif // TRAFFIC_H
