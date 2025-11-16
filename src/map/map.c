#include "map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 1024
#define MAX_TEMP_LINES 1024

bool map_load(Map *map, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        perror("Failed to open map file");
        return false;
    }

    char *lines[MAX_TEMP_LINES];
    int num_lines = 0;
    int width = 0;

    // Read all lines into temporary array to determine dimensions
    char buffer[MAX_LINE_LEN];
    while (fgets(buffer, sizeof(buffer), f))
    {
        // Remove trailing newline
        size_t len = strlen(buffer);
        if (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
        {
            buffer[--len] = '\0';
        }

        if (len == 0)
        {
            continue; // skip empty lines
        }

        if (num_lines >= MAX_TEMP_LINES)
        {
            fprintf(stderr, "Too many lines in map file\n");
            fclose(f);
            return false;
        }

        // Duplicate line into heap memory
        char *line = malloc(len + 1);
        if (!line)
        {
            fclose(f);
            return false;
        }
        strcpy(line, buffer);
        lines[num_lines++] = line;

        if ((int)len > width)
        {
            width = (int)len;
        }
    }

    fclose(f);

    if (num_lines == 0 || width == 0)
    {
        fprintf(stderr, "Map file is empty or invalid\n");
        return false;
    }

    // Initialize actual map structure
    // Allocate map tiles
    map->width = width;
    map->height = num_lines;

    map->tiles = malloc(map->height * sizeof(Tile *));
    if (!map->tiles)
    {
        fprintf(stderr, "Failed to allocate map rows\n");
        goto error_cleanup_lines;
    }

    for (int y = 0; y < map->height; ++y)
    {
        map->tiles[y] = malloc(map->width * sizeof(Tile));
        if (!map->tiles[y])
        {
            fprintf(stderr, "Failed to allocate map row %d\n", y);
            // free previous rows
            for (int k = 0; k < y; ++k)
            {
                free(map->tiles[k]);
            }
            free(map->tiles);
            goto error_cleanup_lines;
        }
    }

    // Fill tiles from lines
    for (int y = 0; y < map->height; ++y)
    {
        char *line = lines[y];
        int line_len = (int)strlen(line);

        for (int x = 0; x < map->width; ++x)
        {
            char c = ' ';
            if (x < line_len)
            {
                c = line[x];
            }
            /* tile_from_char returns an int; store its character value into the Tile.symbol */
            map->tiles[y][x] = tile_from_char(c);
        }
    }

    // Free temporary lines
    for (int i = 0; i < num_lines; ++i)
    {
        free(lines[i]);
    }

    return true;

error_cleanup_lines:
    for (int i = 0; i < num_lines; ++i)
    {
        free(lines[i]);
    }
    return false;
}

void map_free(Map *map)
{
    if (!map || !map->tiles)
        return;

    for (int y = 0; y < map->height; ++y)
    {
        free(map->tiles[y]);
    }
    free(map->tiles);
    map->tiles = NULL;
    map->width = 0;
    map->height = 0;
}

bool map_in_bounds(const Map *map, int x, int y)
{
    return x >= 0 && x < map->width &&
           y >= 0 && y < map->height;
}

bool map_is_walkable(const Map *map, int x, int y)
{
    if (!map_in_bounds(map, x, y))
        return false;
    return map->tiles[y][x].symbol == ' ';
}

void map_print(const Map *map)
{
    for (int y = 0; y < map->height; ++y)
    {
        for (int x = 0; x < map->width; ++x)
        {
            putchar(map->tiles[y][x].symbol);
        }
        putchar('\n');
    }
}