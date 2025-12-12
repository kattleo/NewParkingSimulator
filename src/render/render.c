#include "render.h"

#include <stdio.h>
#include <stdlib.h>

static void clear_screen(void)
{
    printf("\033[2J\033[H");
}

int screen_init(Screen *s, const Map *map)
{
    s->width = map->width;
    s->height = map->height;

    s->buffer = malloc(s->height * sizeof(char *));
    if (!s->buffer)
        return 0;

    for (int y = 0; y < s->height; ++y)
    {
        s->buffer[y] = malloc(s->width * sizeof(char));
        if (!s->buffer[y])
        {
            for (int k = 0; k < y; ++k)
                free(s->buffer[k]);
            free(s->buffer);
            s->buffer = NULL;
            return 0;
        }
    }
    return 1;
}

void screen_free(Screen *s)
{
    if (!s || !s->buffer)
        return;

    for (int y = 0; y < s->height; ++y)
        free(s->buffer[y]);
    free(s->buffer);
    s->buffer = NULL;
    s->width = s->height = 0;
}

void screen_from_map(Screen *s, const Map *map)
{
    for (int y = 0; y < map->height; ++y)
    {
        for (int x = 0; x < map->width; ++x)
        {
            s->buffer[y][x] = map->tiles[y][x].symbol;
        }
    }
}

void screen_draw_vehicle(Screen *s, const Vehicle *v, const Map *map)
{
    (void)map; // not needed right now, but might be useful later

    const Sprite *spr = vehicle_get_sprite(v);
    if (!spr)
        return;

    for (int sy = 0; sy < spr->height; ++sy)
    {
        for (int sx = 0; sx < spr->width; ++sx)
        {
            char c = spr->rows[sy][sx];
            if (c == ' ')
                continue;

            int tx = v->x + sx;
            int ty = v->y + sy;
            if (tx >= 0 && tx < s->width &&
                ty >= 0 && ty < s->height)
            {
                s->buffer[ty][tx] = c;
            }
        }
    }
}

void screen_present(const Screen *s, const Map *map, int step)
{
    clear_screen();
    printf("Step: %d\n", step);

    for (int y = 0; y < s->height; ++y)
    {
        for (int x = 0; x < s->width; ++x)
        {
            // ---- Parking Indicator Logic ----
            Tile *t = &map->tiles[y][x];
            if (t->type == TILE_PARKING_INDICATOR)
            {
                ParkingSpot *spot = t->spot;
                // Only show red if spot is occupied AND the occupant is actually parked
                int is_really_parked = 0;
                if (spot && spot->occupied && spot->occupant && spot->occupant->state == VEH_PARKED)
                    is_really_parked = 1;
                if (is_really_parked)
                    printf("\033[31m|\033[0m"); // red
                else
                    printf("\033[92m│\033[0m"); // bright green

                continue;
            }
            // ---------------------------------

            // Gate rendering: check if this tile is part of a gate (render before map buffer)
            int is_gate_tile = 0;
            int gate_open = 1;
            // Check entry gate
            for (int ti = 0; ti < map->gate_entry.tile_count; ++ti) {
                if (map->gate_entry.xs[ti] == x && map->gate_entry.ys[ti] == y) {
                    is_gate_tile = 1;
                    gate_open = map->gate_entry.open;
                    break;
                }
            }
            // Check exit gate (if not already found)
            if (!is_gate_tile) {
                for (int ti = 0; ti < map->gate_exit.tile_count; ++ti) {
                    if (map->gate_exit.xs[ti] == x && map->gate_exit.ys[ti] == y) {
                        is_gate_tile = 1;
                        gate_open = map->gate_exit.open;
                        break;
                    }
                }
            }
            if (is_gate_tile) {
                if (gate_open)
                    putchar(' ');
                else
                    printf("│");
                continue;
            }

            char c = s->buffer[y][x];

            switch (c)
            {
            case '_':
                printf("─");
                break;
            case '|':
                printf("│");
                break;
            case 'R':
                printf("┌");
                break;
            case 'T':
                printf("┐");
                break;
            case 'L':
                printf("└");
                break;
            case 'J':
                printf("┘");
                break;
            case '+':
                printf("┼");
                break;
            case '*':
                printf("\033[90m*\033[0m");
                break;
            default:
                putchar(c);
                break;
            }
        }
        putchar('\n');
    }
}

void screen_draw_paths(Screen *s, const VehicleList *vehicles)
{
    for (VehicleNode *node = vehicles->head; node != NULL; node = node->next)
    {
        const Vehicle *v = &node->vehicle;
        if (v->path.length <= 0)
            continue;

        for (int i = v->path_index; i < v->path.length; ++i)
        {
            int px = v->path.steps[i].x;
            int py = v->path.steps[i].y;

            if (px >= 0 && px < s->width && py >= 0 && py < s->height)
            {
                if (s->buffer[py][px] == ' ')
                    s->buffer[py][px] = '*';
            }
        }
    }
}
