#include "../common/debug.h"
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

        // --- PARKING LOGIC ---
        // Only consider parking if not already parking or parked
        if (!v->going_to_parking && v->state != VEH_PARKED) {
            // Look for a free parking spot (prefer nearby, fallback to global)
            debug_log("[traffic] Vehicle at (%d,%d) seeking parking (radius=%d)\n", v->x, v->y, 12);
            ParkingSpot *spot = traffic_find_near_free_spot(v, map, 12);
            if (spot && !spot->occupied) {
                v->going_to_parking = 1;
                v->parking_spot_id = spot->id;
                v->assigned_spot = spot;
                spot->occupied = 1;
                spot->occupant = v;
                debug_log("[traffic] Assigned parking spot id=%d anchor=(%d,%d) size=%dx%d\n", spot->id, spot->x0, spot->y0, spot->width, spot->height);
                // Primary: drive to the spot's anchor (upper-left of the block)
                Path p;
                path_init(&p);
                const Sprite *spr = vehicle_get_sprite(v);
                int car_w = spr->width;
                int car_h = spr->height;
                int found = 0;
                debug_log("[traffic] Attempt path to spot anchor (%d,%d)\n", spot->x0, spot->y0);
                if (path_find_with_size(map, v->x, v->y, spot->x0, spot->y0, car_w, car_h, &p)) {
                    vehicle_set_path(v, &p);
                    v->state = VEH_PARKING;
                    debug_log("[traffic] Anchor path success: length=%d\n", p.length);
                    found = 1;
                } else {
                    // Fallback: try any valid position inside the parking area
                    debug_log("[traffic] Anchor path failed; scanning inside spot for alternative positions\n");
                    for (int py = spot->y0; py <= spot->y0 + spot->height - car_h; ++py) {
                        for (int px = spot->x0; px <= spot->x0 + spot->width - car_w; ++px) {
                            if (path_find_with_size(map, v->x, v->y, px, py, car_w, car_h, &p)) {
                                vehicle_set_path(v, &p);
                                v->state = VEH_PARKING;
                                debug_log("[traffic] Fallback path success to (%d,%d): length=%d\n", px, py, p.length);
                                found = 1;
                                break;
                            }
                        }
                        if (found) break;
                    }
                }
                if (!found) {
                    // If no path, give up parking for now
                    debug_log("[traffic] No valid path into spot id=%d; releasing reservation\n", spot->id);
                    v->going_to_parking = 0;
                    v->parking_spot_id = -1;
                    v->assigned_spot = NULL;
                    spot->occupied = 0;
                    spot->occupant = NULL;
                }
            }
        }

        // --- PARKING ARRIVAL/LEAVE LOGIC ---
        if (v->going_to_parking && v->parking_spot_id >= 0 && v->assigned_spot) {
            ParkingSpot *spot = v->assigned_spot;
            // Consider parked when vehicle's anchor reaches the spot's anchor
            int parked = (v->x == spot->x0 && v->y == spot->y0);
            if (parked && !v->has_path) {
                v->state = VEH_PARKED;
                spot->occupied = 1;
                spot->occupant = v;
                debug_log("[traffic] Vehicle parked at spot id=%d anchor=(%d,%d)\n", spot->id, spot->x0, spot->y0);
            }
        }

        // --- PARKING LEAVE LOGIC ---
        if (v->state == VEH_LEAVING && v->assigned_spot) {
            ParkingSpot *spot = v->assigned_spot;
            spot->occupied = 0;
            spot->occupant = NULL;
            v->assigned_spot = NULL;
            v->parking_spot_id = -1;
        }

        // --- WAYPOINT-FOLLOWING LOGIC (if not parking) ---
        if (!v->going_to_parking && v->route_length > 0 && v->route_pos < v->route_length) {
            int target_id = v->route[v->route_pos];
            const Waypoint *w = map_get_waypoint_by_id(map, target_id);
            if (w) {
                const Sprite *spr = vehicle_get_sprite(v);
                int reached = 0;
                for (int sy = 0; sy < spr->height && !reached; ++sy) {
                    for (int sx = 0; sx < spr->width && !reached; ++sx) {
                        char c = spr->rows[sy][sx];
                        if (c == ' ') continue;
                        int tx = v->x + sx;
                        int ty = v->y + sy;
                        if (tx == w->x && ty == w->y) {
                            reached = 1;
                        }
                    }
                }
                if (reached && !v->has_path) {
                    v->route_pos++;
                    if (v->route_pos < v->route_length) {
                        // Plan path to next waypoint
                        int next_id = v->route[v->route_pos];
                        const Waypoint *next_w = map_get_waypoint_by_id(map, next_id);
                        if (next_w) {
                            Path p;
                            path_init(&p);
                            if (path_find(map, v->x, v->y, next_w->x, next_w->y, &p)) {
                                vehicle_set_path(v, &p);
                            }
                        }
                    }
                }
            }
        }
    }

    // Move everyone + collision control
    vehicles_update_all(list, map);
}

ParkingSpot *traffic_find_near_free_spot(Vehicle *v, Map *map, int radius)
{
    ParkingSpot *best = NULL;
    int best_d2 = 999999;

    // Get vehicle bounding box
    const Sprite *spr = vehicle_get_sprite(v);
    int vx0 = v->x;
    int vy0 = v->y;
    int vx1 = vx0 + spr->width - 1;
    int vy1 = vy0 + spr->height - 1;

    for (int i = 0; i < map->parking_count; ++i)
    {
        ParkingSpot *p = &map->parkings[i];

        if (p->occupied)
            continue;

        // Parking area bounding box
        int px0 = p->x0;
        int py0 = p->y0;
        int px1 = px0 + p->width - 1;
        int py1 = py0 + p->height - 1;

        // Compute minimal squared distance between vehicle bbox and parking bbox
        int dx = 0, dy = 0;
        if (vx1 < px0)
            dx = px0 - vx1;
        else if (vx0 > px1)
            dx = vx0 - px1;
        else
            dx = 0;

        if (vy1 < py0)
            dy = py0 - vy1;
        else if (vy0 > py1)
            dy = vy0 - py1;
        else
            dy = 0;

        int dist2 = dx * dx + dy * dy;

        if (dist2 <= radius * radius)
        {
            debug_log("[traffic] Spot id=%d within radius: dist2=%d (occupied=%d)\n", p->id, dist2, p->occupied);
            if (dist2 < best_d2)
            {
                best = p;
                best_d2 = dist2;
            }
        }
    }
    if (best) {
        debug_log("[traffic] Selected nearby spot id=%d (dist2=%d)\n", best->id, best_d2);
        return best;
    }

    // Fallback: pick globally nearest free spot regardless of radius
    best_d2 = 999999;
    for (int i = 0; i < map->parking_count; ++i)
    {
        ParkingSpot *p = &map->parkings[i];

        if (p->occupied)
            continue;

        // measure by anchor distance to favor intended target
        int dx = p->x0 - v->x;
        int dy = p->y0 - v->y;
        int dist2 = dx * dx + dy * dy;

        if (dist2 < best_d2)
        {
            best = p;
            best_d2 = dist2;
        }
    }
    if (best)
        debug_log("[traffic] No spot within radius=%d; selecting global nearest id=%d (dist2=%d)\n", radius, best->id, best_d2);
    else
        debug_log("[traffic] No free parking spots available\n");
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
