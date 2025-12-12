#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "map/map.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_list.h"
#include "render/render.h"
#include "traffic/traffic.h"
#include "common/direction.h"

typedef struct {
    int account_balance;
} Game;

bool assets_init(Map *map)
{
    if (!map_load(map, "assets/map.txt"))
    {
        fprintf(stderr, "Failed to load map\n");
        return false;
    }

    if (!vehicle_sprites_init("assets/carSmall"))
    {
        fprintf(stderr, "Failed to init vehicle sprites\n");
        map_free(map);
        return false;
    }
    return true;
}

int main(void)
{
    Map map;
    if (!assets_init(&map))
        return 1;

    Screen screen;
    if (!screen_init(&screen, &map))
    {
        fprintf(stderr, "Failed to init screen\n");
        map_free(&map);
        return 1;
    }

    VehicleList vehicles;
    vehicle_list_init(&vehicles);

    Game game = {0};
    game.account_balance = 0;

    const int FRAME_DT_MS = 150;
    enum Phase { PHASE_SPAWN, PHASE_WAIT_OPEN, PHASE_OPEN, PHASE_WAIT_CLOSE };
    enum Phase phase = PHASE_SPAWN;
    int phase_timer = 0;
    int vehicle_steps = 0;
    int last_vehicle_x = -1, last_vehicle_y = -1;
    int total_steps = 0;
    int vehicle_id = 0;

    // Ensure gate is closed at start
    map_set_gate_open(&map, 0);

    int car_spawned = 0;
    for (int step = 0; step < 500; ++step) {
        // State machine for gate/vehicle logic
        switch (phase) {
            case PHASE_SPAWN: {
                // Spawn a new vehicle at start
                if (!car_spawned) {
                    Vehicle v;
                    int vx = 133, vy = 27;
                    if (map.has_start) {
                        vx = map.start_x;
                        vy = map.start_y;
                    }
                    vehicle_init(&v, vx, vy, DIR_EAST);
                    vehicle_list_push_back(&vehicles, &v);
                    Vehicle *nv = &vehicles.tail->vehicle;
                    traffic_init_vehicle_route(nv, &map);
                    vehicle_steps = 0;
                    last_vehicle_x = vx;
                    last_vehicle_y = vy;
                    phase_timer = 1000; // 1 second
                    phase = PHASE_WAIT_OPEN;
                    car_spawned = 1;
                }
                break;
            }
            case PHASE_WAIT_OPEN:
                phase_timer -= FRAME_DT_MS;
                if (phase_timer <= 0) {
                    map_set_gate_open(&map, 1); // open gate
                    // Replan path for the most recent vehicle if not parking and has no path
                    VehicleNode *last = vehicles.tail;
                    if (last && !last->vehicle.going_to_parking && !last->vehicle.has_path) {
                        traffic_init_vehicle_route(&last->vehicle, &map);
                    }
                    phase = PHASE_OPEN;
                }
                break;
            case PHASE_OPEN: {
                // Count steps for the most recent vehicle
                VehicleNode *last = vehicles.tail;
                if (last) {
                    // Only count steps if vehicle actually moves
                    if (last->vehicle.x != last_vehicle_x || last->vehicle.y != last_vehicle_y) {
                        vehicle_steps++;
                        last_vehicle_x = last->vehicle.x;
                        last_vehicle_y = last->vehicle.y;
                    }
                    int steps_needed = last->vehicle.sprites->east.width + 2;
                    if (vehicle_steps >= steps_needed) {
                        map_set_gate_open(&map, 0); // close gate
                        phase_timer = 1000; // 1 second
                        phase = PHASE_WAIT_CLOSE;
                    }
                }
                break;
            }
            case PHASE_WAIT_CLOSE:
                phase_timer -= FRAME_DT_MS;
                if (phase_timer <= 0) {
                    phase = PHASE_SPAWN;
                }
                break;
        }

        // 1) Static background
        screen_from_map(&screen, &map);
        screen_draw_paths(&screen, &vehicles);
        // 2) Vehicles
        for (VehicleNode *node = vehicles.head; node != NULL; node = node->next)
            screen_draw_vehicle(&screen, &node->vehicle, &map);

        // (Obsolete VEH_LEAVING back out logic removed; see below for correct per-vehicle logic)
        // 3) Present
        screen_present(&screen, &map, step);

        printf("Account Balance: \033[92m%d\033[0m\n", game.account_balance);
        // --- Stat Board ---
        printf("\n=== Vehicle Overview ===\n");
        printf("%-10s %-12s %-12s\n", "VehicleID", "State", "ParkingTime");
        int vid = 0;
        for (VehicleNode *node = vehicles.head; node != NULL; node = node->next, ++vid) {
            Vehicle *v = &node->vehicle;
            // Increment parking_time if parked
            if (v->state == VEH_PARKED) {
                v->parking_time += FRAME_DT_MS;
                if (v->parking_time >= 3000) {
                    printf("[DEBUG] Vehicle %d: Parking time elapsed, switching to LEAVING.\n", vid);
                    v->state = VEH_LEAVING;
                    v->parking_time = 0;
                    const Sprite *spr = vehicle_get_sprite(v);
                    v->reverse_steps_remaining = spr->width + 2; // Back out 2 extra tiles for testing
                    printf("[DEBUG] Vehicle %d: Starting to reverse out (%d steps)\n", vid, v->reverse_steps_remaining);
                }
            } else {
                v->parking_time = 0;
            }
            const char *state_str = "";
            switch (v->state) {
                case VEH_DRIVING: state_str = "Driving"; break;
                case VEH_PARKING: state_str = "Parking"; break;
                case VEH_PARKED: state_str = "Parked"; break;
                case VEH_LEAVING: state_str = "Leaving"; break;
                default: state_str = "Unknown"; break;
            }
            printf("%-10d %-12s %-12d\n", vid, state_str, v->parking_time);
        }
        // --- Back out logic for VEH_LEAVING ---
        for (VehicleNode *node = vehicles.head; node != NULL; node = node->next) {
            Vehicle *v = &node->vehicle;
            if (v->state == VEH_LEAVING && v->reverse_steps_remaining > 0) {
                printf("[DEBUG] Vehicle at (%d,%d) in LEAVING, reverse_steps_remaining=%d\n", v->x, v->y, v->reverse_steps_remaining);
                // Move in the opposite direction of v->dir
                switch (v->dir) {
                    case DIR_EAST:  v->x -= 1; break;
                    case DIR_WEST:  v->x += 1; break;
                    case DIR_NORTH: v->y += 1; break;
                    case DIR_SOUTH: v->y -= 1; break;
                }
                v->reverse_steps_remaining--;
                printf("[DEBUG] Vehicle: Reversing, steps remaining: %d\n", v->reverse_steps_remaining);
                // Only clear parking assignment after reversing is done
                if (v->reverse_steps_remaining == 0) {
                    // Only clear parking assignment once
                    if (v->assigned_spot) {
                        printf("[DEBUG] Vehicle: Finished reversing, clearing parking spot and searching for exit tile...\n");
                        ParkingSpot *spot = v->assigned_spot;
                        spot->occupied = 0;
                        spot->occupant = NULL;
                        v->assigned_spot = NULL;
                        v->parking_spot_id = -1;
                    }
                    // Always try to plan path to exit if not already driving
                    if (v->state == VEH_LEAVING) {
                        if (map.has_end) {
                            int ex = map.end_x;
                            int ey = map.end_y;
                            const Sprite *spr = vehicle_get_sprite(v);
                            int car_w = spr->width;
                            int car_h = spr->height;
                            printf("[DEBUG] Attempting to pathfind_with_size from (%d, %d) to exit (%d, %d) with car size %dx%d\n", v->x, v->y, ex, ey, car_w, car_h);
                            if (!map_is_walkable(&map, v->x, v->y)) {
                                printf("[DEBUG] Vehicle at (%d,%d) is not on a walkable tile! Tile type: %d\n", v->x, v->y, map.tiles[v->y][v->x].type);
                            }
                            if (!map_is_walkable(&map, ex, ey)) {
                                printf("[DEBUG] Exit tile at (%d,%d) is not walkable! Tile type: %d\n", ex, ey, map.tiles[ey][ex].type);
                            }
                            Path p; path_init(&p);
                            int found = path_find_with_size(&map, v->x, v->y, ex, ey, car_w, car_h, &p);
                            printf("[DEBUG] path_find_with_size returned %d, path length: %d\n", found, p.length);
                            if (found) {
                                vehicle_set_path(v, &p);
                                v->state = VEH_DRIVING;
                                printf("[DEBUG] Path to exit set, vehicle now driving to exit. State: %d, has_path: %d\n", v->state, v->has_path);
                            } else {
                                printf("[DEBUG] Pathfinding to exit failed! Will retry next frame.\n");
                            }
                        } else {
                            printf("[DEBUG] No exit tile found: map.has_end is not set!\n");
                        }
                    }
                }
            }
        }
        // 4) One traffic simulation step (move + path replanning)
        traffic_step(&vehicles, &map);
        usleep(FRAME_DT_MS * 1000);
        total_steps++;
    }

    vehicle_list_clear(&vehicles);
    screen_free(&screen);
    map_free(&map);

    return 0;
}