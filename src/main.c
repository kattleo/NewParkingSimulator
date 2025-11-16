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

    // Example: two cars starting at same position
    Vehicle v1;
    vehicle_init(&v1, 133, 27, DIR_EAST);

    vehicle_list_push_back(&vehicles, &v1);

    // Let traffic module set route & initial paths to waypoints
    traffic_init_routes_waypoints(&vehicles, &map);

    // Game loop
    for (int step = 0; step < 500; ++step)
    {
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