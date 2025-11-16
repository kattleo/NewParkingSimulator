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

void screen_present(const Screen *s, int step)
{
    clear_screen();
    printf("Step: %d\n", step);
    for (int y = 0; y < s->height; ++y)
    {
        fwrite(s->buffer[y], 1, s->width, stdout);
        putchar('\n');
    }
}
