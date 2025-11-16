#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "map/map.h"
#include "vehicle/vehicle.h"
#include "render/render.h"
#include "vehicle/vehicle_list.h"

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

    // Init Vehicle List
    VehicleList vehicles;
    vehicle_list_init(&vehicles);

    // Create some sample cars
    Vehicle v1, v2;
    vehicle_init(&v1, 10, 1, DIR_EAST);
    vehicle_init(&v2, 30, 1, DIR_WEST);

    // Add them to the list
    vehicle_list_push_back(&vehicles, &v1);
    vehicle_list_push_back(&vehicles, &v2);

    for (int step = 0; step < 50; ++step)
    {
        // Get screen from static map
        screen_from_map(&screen, &map);

        // Draw all vehicles to screen
        for (VehicleNode *node = vehicles.head; node != NULL; node = node->next)
        {
            screen_draw_vehicle(&screen, &node->vehicle, &map);
        }

        // Render screen
        screen_present(&screen, step);

        // Update all vehicles
        for (VehicleNode *node = vehicles.head; node != NULL; node = node->next)
        {
            vehicles_update_all(&vehicles, &map);
        }

        usleep(150000);
    }

    vehicle_list_clear(&vehicles);
    screen_free(&screen);
    map_free(&map);

    return 0;
}