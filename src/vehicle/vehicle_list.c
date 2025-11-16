#include "vehicle_list.h"

#include <stdlib.h>

#define OCC_IDX(x, y, width) ((y) * (width) + (x))

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
    int width = map->width;
    int height = map->height;
    int size = width * height;
    Vehicle **occupied = malloc(size * sizeof(Vehicle *));

    if (!occupied)
        return;

    for (int i = 0; i < size; ++i)
    {
        occupied[i] = NULL;
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
                    occupied[OCC_IDX(tx, ty, width)] = v;
                }
            }
        }
    }

    // For each vehicle: try to move with map + car collision
    for (VehicleNode *node = list->head; node != NULL; node = node->next)
    {
        Vehicle *v = &node->vehicle;
        const Sprite *spr = vehicle_get_sprite(v);
        if (!spr)
            continue;

        // No path or finished path → skip movement
        if (!v->has_path || v->path_index >= v->path.length)
        {
            v->has_path = 0;
            continue;
        }

        // 3.1 Remove this car's old footprint from occupancy
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
                    int idx = OCC_IDX(tx, ty, mapWidth);
                    if (occupied[idx] == v)
                    {
                        occupied[idx] = NULL;
                    }
                }
            }
        }

        // Next target tile from path
        PathStep next = v->path.steps[v->path_index];

        int dx = next.x - v->x;
        int dy = next.y - v->y;

        // Direction from path step
        if (dx == 1 && dy == 0)
            v->dir = DIR_EAST;
        else if (dx == -1 && dy == 0)
            v->dir = DIR_WEST;
        else if (dx == 0 && dy == -1)
            v->dir = DIR_NORTH;
        else if (dx == 0 && dy == 1)
            v->dir = DIR_SOUTH;
        // If dx,dy are weird (e.g. path broken), we could bail:
        // else { v->has_path = 0; goto re_mark_old_footprint; }

        int new_x = next.x;
        int new_y = next.y;

        int blocked = 0;

        // Combined map + car collision check at new_x,new_y
        for (int sy = 0; sy < spr->height && !blocked; ++sy)
        {
            for (int sx = 0; sx < spr->width; ++sx)
            {
                char c = spr->rows[sy][sx];
                if (c == ' ')
                    continue;

                int tx = new_x + sx;
                int ty = new_y + sy;

                // Map collision
                if (!map_is_walkable(map, tx, ty))
                {
                    blocked = 1;
                    break;
                }

                // Car–car collision
                if (tx >= 0 && tx < mapWidth &&
                    ty >= 0 && ty < mapHeight)
                {
                    int idx = OCC_IDX(tx, ty, mapWidth);
                    if (occupied[idx] != NULL)
                    {
                        blocked = 1;
                        break;
                    }
                }
                else
                {
                    // Out of bounds, treat as blocked
                    blocked = 1;
                    break;
                }
            }
        }

        if (!blocked)
        {
            // Move is valid → apply it
            v->x = new_x;
            v->y = new_y;
            v->path_index++;

            if (v->path_index >= v->path.length)
            {
                v->has_path = 0; // reached goal
            }
        }
        // else: car stays where it is (v->x, v->y unchanged, path_index unchanged)

        // Mark this car's footprint back into occupancy
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
                    occupied[OCC_IDX(tx, ty, mapWidth)] = v;
                }
            }
        }
    }

    // Free occupancy grid
    for (int y = 0; y < mapHeight; ++y)
    {
        free(occupied[y]);
    }
    free(occupied);
}
