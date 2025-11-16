#include "traffic.h"

#include <stdio.h>

// Helper: set default route 1..N for a single vehicle
static void vehicle_set_default_route(Vehicle *v, int num_waypoints)
{
    v->route_length = num_waypoints;
    v->route_pos = 0;

    if (v->route_length > MAX_ROUTE_WAYPOINTS)
        v->route_length = MAX_ROUTE_WAYPOINTS;

    for (int i = 0; i < v->route_length; ++i)
        v->route[i] = i + 1; // waypoint IDs: 1,2,3,...
}

// Helper: plan path from vehicle's current pos to current route waypoint
static void vehicle_plan_path_to_current_waypoint(Vehicle *v, Map *map)
{
    if (v->route_length == 0 || v->route_pos >= v->route_length)
        return;

    int target_id = v->route[v->route_pos];
    const Waypoint *w = map_get_waypoint_by_id(map, target_id);
    if (!w)
        return;

    Path p;
    path_init(&p);

    if (path_find(map, v->x, v->y, w->x, w->y, &p))
    {
        vehicle_set_path(v, &p);
    }
    else
    {
        v->has_path = 0;
    }
}

void traffic_init_routes_waypoints(VehicleList *vehicles, Map *map)
{
    int num_waypoints = map->waypoint_count;
    if (num_waypoints <= 0)
        return;

    for (VehicleNode *node = vehicles->head; node != NULL; node = node->next)
    {
        Vehicle *v = &node->vehicle;

        // 1) assign route 1..N
        vehicle_set_default_route(v, num_waypoints);

        // 2) plan initial path to route[0]
        vehicle_plan_path_to_current_waypoint(v, map);
    }
}

void traffic_step(VehicleList *vehicles, Map *map)
{
    // 1) Move all vehicles along their current paths + handle collisions
    vehicles_update_all(vehicles, map);

    // 2) For vehicles that finished their path but still have waypoints, plan next segment
    for (VehicleNode *node = vehicles->head; node != NULL; node = node->next)
    {
        Vehicle *v = &node->vehicle;

        if (!v->has_path)
        {
            // still waypoints left?
            if (v->route_pos + 1 < v->route_length)
            {
                v->route_pos++;
                vehicle_plan_path_to_current_waypoint(v, map);
            }
            // else: route completed, car stays where it is
        }
    }
}
