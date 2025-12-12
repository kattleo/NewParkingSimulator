// ...existing includes...
#include "common/debug.h"


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "common/game.h"
#include <string.h>
#include "common/menu.h"
#include "common/menu.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_list.h"
#include "render/render.h"
#include "traffic/traffic.h"
#include "common/direction.h"



bool assets_init(Map *map)
{
    if (!map_load(map, "assets/map.txt"))
    {
        debug_log("Failed to load map\n");
        return false;
    }

    if (!vehicle_sprites_init("assets/carSmall"))
    {
        debug_log("Failed to init vehicle sprites\n");
        map_free(map);
        return false;
    }
    return true;
}

// Add a field to Vehicle for real-time parking start (in ms since epoch)
#include <stdint.h>

// Add to Vehicle struct in vehicle.h:
// uint64_t parking_start_time_ms;

// Helper to get current time in ms
static uint64_t now_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main(void)
{
    // Show menu before starting game
    int mode = menu_show(); // 0 = Smooth, 1 = Busy
    Config config;
    config_load(&config, "assets/config.txt");
    // Set selected mode's parking times, spawn rate, and frame duration
    if (mode == 0) {
        config.min_parking_time_sec = config.min_parking_time_smooth;
        config.max_parking_time_sec = config.max_parking_time_smooth;
        config.spawn_rate_ms = config.spawn_rate_smooth;
        config.frame_dt_ms = config.frame_dt_ms_smooth;
    } else {
        config.min_parking_time_sec = config.min_parking_time_busy;
        config.max_parking_time_sec = config.max_parking_time_busy;
        config.spawn_rate_ms = config.spawn_rate_busy;
        config.frame_dt_ms = config.frame_dt_ms_busy;
    }
    debug_set_enabled(config.debug_logs);

    // Start looping street ambience sound
    system("play -q assets/sounds/street_ambience.mp3 repeat 9999 > /dev/null 2>&1 &");
    Map map;
    if (!assets_init(&map))
        return 1;

    Screen screen;
    if (!screen_init(&screen, &map))
    {
        debug_log("Failed to init screen\n");
        map_free(&map);
        return 1;
    }

    VehicleList vehicles;
    vehicle_list_init(&vehicles);

    Game game = {0};
    game.account_balance = 0;

    const int FRAME_DT_MS = config.frame_dt_ms;
    enum Phase { PHASE_SPAWN, PHASE_WAIT_OPEN, PHASE_OPEN, PHASE_WAIT_CLOSE, PHASE_WAIT_SPAWN };
    enum Phase phase = PHASE_SPAWN;
    int phase_timer = 0;
    int vehicle_steps = 0;
    int last_vehicle_x = -1, last_vehicle_y = -1;
    int total_steps = 0;
    int vehicle_id = 0;

    // Ensure gate is closed at start
    map_set_gate_open(&map, 0);

    if (config.show_intro) {
        FILE *logo = fopen("assets/logo.txt", "r");
        if (logo) {
            char buf[128];
            while (fgets(buf, sizeof(buf), logo)) fputs(buf, stdout);
            fclose(logo);
            printf("\n");
        }
    }

    for (int step = 0; step < 500; ++step) {
        // State machine for gate/vehicle logic
        switch (phase) {
            case PHASE_SPAWN: {
                // Spawn a new vehicle at start
                Vehicle v;
                int vx = 133, vy = 27;
                if (map.has_start) {
                    vx = map.start_x;
                    vy = map.start_y;
                }
                vehicle_init(&v, vx, vy, DIR_WEST);
                vehicle_list_push_back(&vehicles, &v);
                Vehicle *nv = &vehicles.tail->vehicle;
                traffic_init_vehicle_route(nv, &map);
                vehicle_steps = 0;
                last_vehicle_x = vx;
                last_vehicle_y = vy;
                phase_timer = config.spawn_rate_ms; // Use mode-specific spawn rate
                phase = PHASE_WAIT_OPEN;
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
                phase_timer -= FRAME_DT_MS / 2; // match tick speed
                if (phase_timer <= 0) {
                    phase = PHASE_WAIT_SPAWN;
                    phase_timer = 1000; // 1 second wait before next spawn
                }
                break;
            case PHASE_WAIT_SPAWN:
                phase_timer -= FRAME_DT_MS / 2;
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
        printf("%-10s %-12s %-12s\n", "VehicleID", "State", "ParkingTime (s)");
            printf("%-10s %-12s %-15s %-15s\n", "VehicleID", "State", "ParkingTime (s)", "Remaining (s)");
        int vid = 0;
        for (VehicleNode *node = vehicles.head; node != NULL; node = node->next, ++vid) {
            Vehicle *v = &node->vehicle;
            // Assign parking time and set real start time
            if (v->state == VEH_PARKED) {
                if (v->parking_time_sec == 0) {
                    int min_sec = config.min_parking_time_sec;
                    int max_sec = config.max_parking_time_sec;
                    if (max_sec < min_sec) max_sec = min_sec;
                    v->parking_time_sec = min_sec + rand() % (max_sec - min_sec + 1);
                    v->parking_start_time_ms = now_ms();
                    debug_log("[DEBUG] Vehicle %d: Assigned random parking time: %d s, start_time_ms: %llu\n", vid, v->parking_time_sec, (unsigned long long)v->parking_start_time_ms);
                }
                uint64_t elapsed_ms = now_ms() - v->parking_start_time_ms;
                int remaining_ms = v->parking_time_sec * 1000 - (int)elapsed_ms;
                if (remaining_ms < 0) remaining_ms = 0;
                v->parking_time_remaining = remaining_ms;
                if (v->parking_time_remaining <= 0) {
                    debug_log("[DEBUG] Vehicle %d: Parking time elapsed, switching to LEAVING.\n", vid);
                    v->state = VEH_LEAVING;
                    const Sprite *spr = vehicle_get_sprite(v);
                    v->reverse_steps_remaining = spr->width + 2; // Back out 2 extra tiles for testing
                    debug_log("[DEBUG] Vehicle %d: Starting to reverse out (%d steps)\n", vid, v->reverse_steps_remaining);
                }
            } else {
                // Reset for next time parked
                v->parking_time_sec = (v->state == VEH_LEAVING || v->state == VEH_EXIT_QUEUE || v->state == VEH_DRIVING) ? v->parking_time_sec : 0;
            }
            const char *state_str = "";
            switch (v->state) {
                case VEH_DRIVING: state_str = "Driving"; break;
                case VEH_PARKING: state_str = "Parking"; break;
                case VEH_PARKED: state_str = "Parked"; break;
                case VEH_LEAVING: state_str = "Leaving"; break;
                case VEH_EXIT_QUEUE: state_str = "ExitQueue"; break;
                default: state_str = "Unknown"; break;
            }
            int remaining_sec = (v->state == VEH_PARKED) ? (v->parking_time_remaining + 999) / 1000 : 0;
            printf("%-10d %-12s %-15d %-15d\n", vid, state_str, v->parking_time_sec, remaining_sec);
        }
        // --- Back out logic for VEH_LEAVING ---
        for (VehicleNode *node = vehicles.head; node != NULL; node = node->next) {
            Vehicle *v = &node->vehicle;
            // When vehicle reaches (0,1), close the gate again
            if (v->state == VEH_DRIVING && v->x == 0 && v->y == 1) {
                if (map.gate_exit.open) {
                    map.gate_exit.open = 0;
                    debug_log("[DEBUG] Exit gate closed after vehicle reached (0,1).\n");
                }
                // Add money to account based on parking_time_sec and mark for deletion
                int payout = v->parking_time_sec * 10;
                game.account_balance += payout;
                system("play assets/sounds/money_count.mp3 > /dev/null 2>&1 &");
                debug_log("[DEBUG] Vehicle at (0,1) exited. +%d to account for %d seconds parked. Marking for removal.\n", payout, v->parking_time_sec);
                v->state = -1; // Mark for deletion
            }
            // Handle exit gate opening for single vehicle
            // Transition to exit queue and immediately assign path if vehicle reaches 'E' tile (exit entry spot)
            if ((v->state == VEH_DRIVING || v->state == VEH_EXIT_QUEUE) && map.has_end && v->x == map.end_x && v->y == map.end_y) {
                if (!map.gate_exit.open) {
                    map.gate_exit.open = 1;
                    debug_log("[DEBUG] Exit gate opened for vehicle %d.\n", vid);
                }
                v->state = VEH_EXIT_QUEUE;
                v->has_path = 0;
                debug_log("[DEBUG] Vehicle %d: Reached exit entry spot ('E'), now in exit queue.\n", vid);
                // Set path goal to (0,1)
                int target_x = 0, target_y = 1;
                const Sprite *spr = vehicle_get_sprite(v);
                debug_log("[DEBUG] Car sprite width: %d, height: %d\n", spr ? spr->width : -1, spr ? spr->height : -1);
                Path p; path_init(&p);
                int found = path_find(&map, v->x, v->y, target_x, target_y, &p);
                debug_log("[DEBUG] path_find to (%d,%d) returned %d, path length: %d\n", target_x, target_y, found, p.length);
                if (found) {
                    vehicle_set_path(v, &p);
                    v->state = VEH_DRIVING;
                    debug_log("[DEBUG] Vehicle %d: Path to (%d,%d) set, now driving to exit (0,1).\n", vid, target_x, target_y);
                } else {
                    debug_log("[DEBUG] Vehicle %d: Failed to set path to (%d,%d)!\n", vid, target_x, target_y);
                }
            }
            // When vehicle reaches (0,1), close the gate again
            if (v->state == VEH_DRIVING && v->x == 0 && v->y == 1) {
                if (map.gate_exit.open) {
                    map.gate_exit.open = 0;
                    debug_log("[DEBUG] Exit gate closed after vehicle reached (0,1).\n");
                }
            }
            if (v->state == VEH_LEAVING && v->reverse_steps_remaining > 0) {
                debug_log("[DEBUG] Vehicle at (%d,%d) in LEAVING, reverse_steps_remaining=%d\n", v->x, v->y, v->reverse_steps_remaining);
                // Move in the opposite direction of v->dir
                switch (v->dir) {
                    case DIR_EAST:  v->x -= 1; break;
                    case DIR_WEST:  v->x += 1; break;
                    case DIR_NORTH: v->y += 1; break;
                    case DIR_SOUTH: v->y -= 1; break;
                }
                v->reverse_steps_remaining--;
                debug_log("[DEBUG] Vehicle: Reversing, steps remaining: %d\n", v->reverse_steps_remaining);
                // Only clear parking assignment after reversing is done
                if (v->reverse_steps_remaining == 0) {
                    // Only clear parking assignment once
                    if (v->assigned_spot) {
                        debug_log("[DEBUG] Vehicle: Finished reversing, clearing parking spot and searching for exit tile...\n");
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
                            debug_log("[DEBUG] Attempting to pathfind_with_size from (%d, %d) to exit (%d, %d) with car size %dx%d\n", v->x, v->y, ex, ey, car_w, car_h);
                            if (!map_is_walkable(&map, v->x, v->y)) {
                                debug_log("[DEBUG] Vehicle at (%d,%d) is not on a walkable tile! Tile type: %d\n", v->x, v->y, map.tiles[v->y][v->x].type);
                            }
                            if (!map_is_walkable(&map, ex, ey)) {
                                debug_log("[DEBUG] Exit tile at (%d,%d) is not walkable! Tile type: %d\n", ex, ey, map.tiles[ey][ex].type);
                            }
                            Path p; path_init(&p);
                            int found = path_find_with_size(&map, v->x, v->y, ex, ey, car_w, car_h, &p);
                            debug_log("[DEBUG] path_find_with_size returned %d, path length: %d\n", found, p.length);
                            if (found) {
                                vehicle_set_path(v, &p);
                                v->state = VEH_DRIVING;
                                debug_log("[DEBUG] Path to exit set, vehicle now driving to exit. State: %d, has_path: %d\n", v->state, v->has_path);
                            } else {
                                debug_log("[DEBUG] Pathfinding to exit failed! Will retry next frame.\n");
                            }
                        } else {
                            debug_log("[DEBUG] No exit tile found: map.has_end is not set!\n");
                        }
                    }
                }
            }
        }
        // 4) One traffic simulation step (move + path replanning)
        traffic_step(&vehicles, &map);
        usleep(FRAME_DT_MS * 500); // double speed for debugging
        total_steps++;
            // Remove vehicles marked for deletion
            VehicleNode *prev = NULL;
            VehicleNode *node = vehicles.head;
            while (node) {
                Vehicle *v = &node->vehicle;
                VehicleNode *next = node->next;
                if (v->state == -1) {
                    if (prev) prev->next = next;
                    else vehicles.head = next;
                    if (vehicles.tail == node) vehicles.tail = prev;
                    free(node);
                    node = next;
                    continue;
                }
                prev = node;
                node = next;
            }
    }

    vehicle_list_clear(&vehicles);
    screen_free(&screen);
    map_free(&map);

    return 0;
}