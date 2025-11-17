#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "map/map.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_list.h"
#include "render/render.h"
#include "traffic/traffic.h"

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

    int spawn_timer_ms = 0; // first car spawns immediately
    const int FRAME_DT_MS = 150;

    // Game loop
    for (int step = 0; step < 500; ++step)
    {
        // 0) Spawning logic: one car every 5 seconds
        spawn_timer_ms -= FRAME_DT_MS;
        if (spawn_timer_ms <= 0)
        {
            Vehicle v;
            // same position & direction as your old v1: (133, 27), DIR_EAST
            vehicle_init(&v, 133, 27, DIR_EAST);

            // add to list (copied into node->vehicle)
            vehicle_list_push_back(&vehicles, &v);

            // assign routes & initial paths for cars
            Vehicle *nv = &vehicles.tail->vehicle;
            traffic_init_vehicle_route(nv, &map);

            // schedule next spawn in 5 seconds (5000 ms)
            spawn_timer_ms = 5000;
        }

        // 1) Static background
        screen_from_map(&screen, &map);

        screen_draw_paths(&screen, &vehicles);

        // 2) Vehicles
        for (VehicleNode *node = vehicles.head; node != NULL; node = node->next)
        {
            screen_draw_vehicle(&screen, &node->vehicle, &map);
        }

        // 3) Present
        screen_present(&screen, step);

        // 4) One traffic simulation step (move + path replanning)
        traffic_step(&vehicles, &map);

        usleep(150000);
    }

    vehicle_list_clear(&vehicles);
    screen_free(&screen);
    map_free(&map);

    return 0;
}