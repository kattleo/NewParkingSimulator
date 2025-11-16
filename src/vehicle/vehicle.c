#include "vehicle.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static VehicleSprites g_default_sprites;
static int g_sprites_loaded = 0;

// dimensions for CarSmall
#define CAR_SMALL_HORIZONTAL_WIDTH 8
#define CAR_SMALL_HORIZONTAL_HEIGHT 2
#define CAR_SMALL_VERTICAL_WIDTH 3
#define CAR_SMALL_VERTICAL_HEIGHT 3

static int load_sprite_from_file(Sprite *spr,
                                 const char *filepath,
                                 int width,
                                 int height)
{
    FILE *f = fopen(filepath, "r");
    if (!f)
    {
        fprintf(stderr, "Failed to open sprite file: %s\n", filepath);
        return 0;
    }

    spr->width = width;
    spr->height = height;

    spr->rows = malloc(height * sizeof(char *));
    if (!spr->rows)
    {
        fclose(f);
        return 0;
    }

    char buffer[1024];

    for (int y = 0; y < height; ++y)
    {
        if (!fgets(buffer, sizeof(buffer), f))
        {
            fprintf(stderr, "Sprite file %s has too few lines (expected %d)\n",
                    filepath, height);
            // cleanup
            for (int k = 0; k < y; ++k)
                free(spr->rows[k]);
            free(spr->rows);
            spr->rows = NULL;
            fclose(f);
            return 0;
        }

        // Remove newline
        size_t len = strlen(buffer);
        if (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
        {
            buffer[--len] = '\0';
        }

        if ((int)len < width)
        {
            fprintf(stderr, "Sprite file %s line %d too short (got %zu, need %d)\n",
                    filepath, y, len, width);
            for (int k = 0; k < y; ++k)
                free(spr->rows[k]);
            free(spr->rows);
            spr->rows = NULL;
            fclose(f);
            return 0;
        }

        spr->rows[y] = malloc(width * sizeof(char));
        if (!spr->rows[y])
        {
            for (int k = 0; k < y; ++k)
                free(spr->rows[k]);
            free(spr->rows);
            spr->rows = NULL;
            fclose(f);
            return 0;
        }

        // Copy the first "width" characters from the line
        memcpy(spr->rows[y], buffer, width);
    }

    fclose(f);
    return 1;
}

bool vehicle_sprites_init(const char *base_path)
{
    char path[512];

    // North
    snprintf(path, sizeof(path), "%s/carSmall_N.txt", base_path);
    if (!load_sprite_from_file(&g_default_sprites.north,
                               path,
                               CAR_SMALL_VERTICAL_WIDTH,
                               CAR_SMALL_VERTICAL_HEIGHT))
    {
        return false;
    }

    // East
    snprintf(path, sizeof(path), "%s/carSmall_E.txt", base_path);
    if (!load_sprite_from_file(&g_default_sprites.east,
                               path,
                               CAR_SMALL_HORIZONTAL_WIDTH,
                               CAR_SMALL_HORIZONTAL_HEIGHT))
    {
        return false;
    }

    // South
    snprintf(path, sizeof(path), "%s/carSmall_S.txt", base_path);
    if (!load_sprite_from_file(&g_default_sprites.south,
                               path,
                               CAR_SMALL_VERTICAL_WIDTH,
                               CAR_SMALL_VERTICAL_HEIGHT))
    {
        return false;
    }

    // West
    snprintf(path, sizeof(path), "%s/carSmall_W.txt", base_path);
    if (!load_sprite_from_file(&g_default_sprites.west,
                               path,
                               CAR_SMALL_HORIZONTAL_WIDTH,
                               CAR_SMALL_HORIZONTAL_HEIGHT))
    {
        return false;
    }

    g_sprites_loaded = 1;
    return true;
}

const VehicleSprites *vehicle_sprites_get_default(void)
{
    if (!g_sprites_loaded)
    {
        fprintf(stderr, "Error: vehicle sprites not initialized!\n");
        return NULL;
    }
    return &g_default_sprites;
}

void vehicle_init(Vehicle *v, int x, int y, Direction dir)
{
    v->x = x;
    v->y = y;
    v->dir = dir;
    v->sprites = vehicle_sprites_get_default();
}

void vehicle_update(Vehicle *v, const Map *map)
{
    int dx = 0;
    int dy = 0;

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

    const Sprite *spr = vehicle_get_sprite(v);

    // Check if the whole sprite footprint fits on walkable map tiles
    for (int sy = 0; sy < spr->height; ++sy)
    {
        for (int sx = 0; sx < spr->width; ++sx)
        {
            char c = spr->rows[sy][sx];
            if (c == ' ')
                continue; // transparent

            int tx = new_x + sx;
            int ty = new_y + sy;

            if (!map_is_walkable(map, tx, ty))
            {
                // Blocked by wall or out of bounds -> don't move
                return;
            }
        }
    }

    // All covered tiles are walkable -> Update position
    v->x = new_x;
    v->y = new_y;
}

const Sprite *vehicle_get_sprite(const Vehicle *v)
{
    switch (v->dir)
    {
    case DIR_NORTH:
        return &v->sprites->north;
    case DIR_EAST:
        return &v->sprites->east;
    case DIR_SOUTH:
        return &v->sprites->south;
    case DIR_WEST:
        return &v->sprites->west;
    }
    return &v->sprites->east;
}