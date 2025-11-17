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

void traffic_init_vehicle_route(Vehicle *v, Map *map)
{
    int num_waypoints = map->waypoint_count;
    if (num_waypoints <= 0)
        return;

    vehicle_set_default_route(v, num_waypoints);
    vehicle_plan_path_to_current_waypoint(v, map);
}

void traffic_step(VehicleList *list, Map *map)
{
    for (VehicleNode *node = list->head; node; node = node->next)
    {
        Vehicle *v = &node->vehicle;

        // Already assigned to a parking spot?
        if (v->state == VEH_DRIVING && !v->wants_parking)
        {
            ParkingSpot *spot = traffic_find_near_free_spot(v, map, 8);

            if (spot)
            {
                printf("Vehicle %d: found parking spot at (%d,%d)\n",
                       spot->x0, spot->y0);

                v->wants_parking = true;
                v->assigned_spot = spot;
                spot->occupied = true;
                spot->occupant = v;

                // Create path to the anchor
                Path p;
                path_init(&p);
                if (path_find(map, v->x, v->y, spot->x0, spot->y0, &p))
                {
                    vehicle_set_path(v, &p);
                    v->state = VEH_PARKING;
                    continue; // skip waypoint logic
                }
                else
                {
                    // can't reach it, make it free again
                    spot->occupied = false;
                    spot->occupant = NULL;
                    v->wants_parking = false;
                    v->assigned_spot = NULL;
                }
            }
        }

        // If vehicle is parking and has reached its spot
        if (v->state == VEH_PARKING && !v->has_path)
        {
            v->state = VEH_PARKED;
            printf("Vehicle parked!\n");
            continue;
        }

        //  OTHERWISE: old waypoint-driving logic …
    }

    // Move everyone + collision control
    vehicles_update_all(list, map);
}

ParkingSpot *traffic_find_near_free_spot(Vehicle *v, Map *map, int radius)
{
    ParkingSpot *best = NULL;
    int best_d2 = 999999;

    for (int i = 0; i < map->parking_count; ++i)
    {
        ParkingSpot *p = &map->parkings[i];

        if (p->occupied)
            continue;

        int dx = p->x0 - v->x;
        int dy = p->y0 - v->y;

        int dist2 = dx * dx + dy * dy;

        if (dist2 <= radius * radius)
        {
            if (dist2 < best_d2)
            {
                best = p;
                best_d2 = dist2;
            }
        }
    }
    return best;
}

void traffic_update_parking_states(Vehicle *v, Map *map)
{
    if (!v->going_to_parking || v->parking_spot_id < 0)
        return;

    if (v->has_path) // still moving
        return;

    // arrived at parking anchor
    v->state = VEH_PARKED;
    // keep spot->occupied = 1
}
