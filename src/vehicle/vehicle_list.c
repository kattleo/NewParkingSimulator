#include "vehicle_list.h"

#include <stdlib.h>

void vehicle_list_init(VehicleList *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

Vehicle *vehicle_list_push_back(VehicleList *list, const Vehicle *vehicleToAdd)
{
    VehicleNode *node = malloc(sizeof(VehicleNode));
    if (!node)
    {
        return NULL;
    }

    node->vehicle = *vehicleToAdd; // copy struct by value
    node->next = NULL;

    if (!list->head)
    {
        // first node in the list
        list->head = node;
        list->tail = node;
    }
    else
    {
        list->tail->next = node;
        list->tail = node;
    }

    list->size++;
    return &node->vehicle;
}

void vehicle_list_clear(VehicleList *list)
{
    VehicleNode *cur = list->head;
    while (cur)
    {
        VehicleNode *next = cur->next;
        free(cur);
        cur = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void vehicles_update_all(VehicleList *list, Map *map)
{
    int mapWidth = map->width;
    int mapHeight = map->height;

    // Allocate occupancy grid to check for collisions
    // This grid either holds a Pointer to a Vehicle or Null
    Vehicle ***occupied = malloc(mapHeight * sizeof(Vehicle **));
    if (!occupied)
        return;

    for (int y = 0; y < mapHeight; ++y)
    {
        occupied[y] = malloc(mapWidth * sizeof(Vehicle *));
        if (!occupied[y])
        {
            for (int k = 0; k < y; ++k)
                free(occupied[k]);
            free(occupied);
            return;
        }
    }
    for (int y = 0; y < mapHeight; ++y)
    {
        for (int x = 0; x < mapWidth; ++x)
        {
            occupied[y][x] = NULL;
        }
    }

    // Build initial occupancy from current positions (before movement)
    for (VehicleNode *node = list->head; node != NULL; node = node->next)
    {
        Vehicle *v = &node->vehicle;
        const Sprite *spr = vehicle_get_sprite(v);
        if (!spr)
            continue;

        for (int sy = 0; sy < spr->height; ++sy)
        {
            for (int sx = 0; sx < spr->width; ++sx)
            {
                char c = spr->rows[sy][sx];
                if (c == ' ')
                    continue;

                int tx = v->x + sx;
                int ty = v->y + sy;

                if (tx >= 0 && tx < mapWidth &&
                    ty >= 0 && ty < mapHeight)
                {
                    occupied[ty][tx] = v;
                }
            }
        }
    }

    // For each vehicle: try to move with map + car collision
    for (VehicleNode *node = list->head; node != NULL; node = node->next)
    {
        Vehicle *v = &node->vehicle;
        const Sprite *vehicleSprite = vehicle_get_sprite(v);
        if (!vehicleSprite)
            continue;

        // Remove this car's old footprint from occupancy (so it doesn't collide with itself)
        for (int sy = 0; sy < vehicleSprite->height; ++sy)
        {
            for (int sx = 0; sx < vehicleSprite->width; ++sx)
            {
                char c = vehicleSprite->rows[sy][sx];
                if (c == ' ')
                    continue;

                int tx = v->x + sx;
                int ty = v->y + sy;

                if (tx >= 0 && tx < mapWidth &&
                    ty >= 0 && ty < mapHeight)
                {
                    if (occupied[ty][tx] == v)
                    {
                        occupied[ty][tx] = NULL;
                    }
                }
            }
        }

        // Compute proposed new position from direction
        int dx = 0, dy = 0;
        switch (v->dir)
        {
        case DIR_EAST:
            dx = 1;
            dy = 0;
            break;
        case DIR_WEST:
            dx = -1;
            dy = 0;
            break;
        case DIR_NORTH:
            dx = 0;
            dy = -1;
            break;
        case DIR_SOUTH:
            dx = 0;
            dy = 1;
            break;
        }

        int new_x = v->x + dx;
        int new_y = v->y + dy;

        int blocked = 0;

        // Check for collissions
        for (int sy = 0; sy < vehicleSprite->height && !blocked; ++sy)
        {
            for (int sx = 0; sx < vehicleSprite->width; ++sx)
            {
                char c = vehicleSprite->rows[sy][sx];
                if (c == ' ')
                    continue;

                int tx = new_x + sx;
                int ty = new_y + sy;

                // Map bcollision
                if (!map_is_walkable(map, tx, ty))
                {
                    blocked = 1;
                    break;
                }

                // Car collision
                if (occupied[ty][tx] != NULL)
                {
                    blocked = 1;
                    break;
                }
            }
        }

        if (!blocked)
        {
            // Move is valid, update position
            v->x = new_x;
            v->y = new_y;
        }
        // else: blocked, car stays where it is

        // Mark this car's footprint back into occupancy
        for (int sy = 0; sy < vehicleSprite->height; ++sy)
        {
            for (int sx = 0; sx < vehicleSprite->width; ++sx)
            {
                char c = vehicleSprite->rows[sy][sx];
                if (c == ' ')
                    continue;

                int tx = v->x + sx;
                int ty = v->y + sy;

                if (tx >= 0 && tx < mapWidth &&
                    ty >= 0 && ty < mapHeight)
                {
                    occupied[ty][tx] = v;
                }
            }
        }
    }

    // Free occupancy grid ---
    for (int y = 0; y < mapHeight; ++y)
    {
        free(occupied[y]);
    }
    free(occupied);
}
