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
                vehicle_init(&v, vx, vy, DIR_EAST);
                vehicle_list_push_back(&vehicles, &v);
                Vehicle *nv = &vehicles.tail->vehicle;
                traffic_init_vehicle_route(nv, &map);
                vehicle_steps = 0;
                last_vehicle_x = vx;
                last_vehicle_y = vy;
                phase_timer = 1000; // 1 second
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
        // 3) Present
        screen_present(&screen, &map, step);

        printf("Account Balance: \033[92m%d\033[0m\n", game.account_balance);
        // --- Stat Board ---
        printf("\n=== Vehicle Overview ===\n");
        printf("%-10s %-12s %-12s\n", "VehicleID", "State", "ParkingTime");
        int vid = 0;
        for (VehicleNode *node = vehicles.head; node != NULL; node = node->next, ++vid) {
            const Vehicle *v = &node->vehicle;
            const char *state_str = "";
            switch (v->state) {
                case VEH_DRIVING: state_str = "Driving"; break;
                case VEH_PARKING: state_str = "Parking"; break;
                case VEH_PARKED: state_str = "Parked"; break;
                case VEH_LEAVING: state_str = "Leaving"; break;
                default: state_str = "Unknown"; break;
            }
            // Hardcoded parking time for now
            int parking_time = 42;
            printf("%-10d %-12s %-12d\n", vid, state_str, parking_time);
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