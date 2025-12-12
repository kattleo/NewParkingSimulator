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
    map->waypoint_count = 0;
    map->parking_count = 0;

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

            Tile t = tile_from_char(c);

            // waypoint detection
            if (c >= '1' && c <= '9')
            {
                int id = c - '0';

                t.is_waypoint = true;
                t.waypoint_id = id;

                if (map->waypoint_count < MAX_WAYPOINTS)
                {
                    map->waypoints[map->waypoint_count].id = id;
                    map->waypoints[map->waypoint_count].x = x;
                    map->waypoints[map->waypoint_count].y = y;
                    map->waypoint_count++;
                }
            }

            // store tile
            map->tiles[y][x] = t;
        }
    }

    map_build_parking_spots(map);

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

void map_build_parking_spots(Map *map)
{
    map->parking_count = 0;

    for (int y = 0; y < map->height - 1; ++y)
    {
        // Detect parking spots as 2x8 blocks of TILE_PARKING
        for (int x = 0; x < map->width - 7; ++x)
        {
            // Check if a 2×8 block of TILE_PARKING begins here
            int is_parking = 1;

            for (int dy = 0; dy < 2 && is_parking; ++dy)
            {
                for (int dx = 0; dx < 8; ++dx)
                {
                    if (map->tiles[y + dy][x + dx].type != TILE_PARKING)
                    {
                        is_parking = 0;
                        break;
                    }
                }
            }

            if (!is_parking)
                continue;

            // -- We found a parking spot --
            if (map->parking_count < MAX_PARKING_SPOTS)
            {
                ParkingSpot *spot = &map->parkings[map->parking_count];
                spot->id = map->parking_count;

                spot->x0 = x;
                spot->y0 = y;
                spot->width = 8;
                spot->height = 2;

                spot->occupied = 0;
                spot->occupant = NULL;

                // Determine indicator position:
                // Left '('|'') or right
                spot->indicator_x = -1;
                spot->indicator_y = -1;

                // Left side
                if (x > 0 &&
                    map->tiles[y][x - 1].symbol == '|')
                {
                    spot->indicator_x = x - 1;
                    spot->indicator_y = y;
                }
                // Right side
                else if (x + 8 < map->width &&
                         map->tiles[y][x + 8].symbol == '|')
                {
                    spot->indicator_x = x + 8;
                    spot->indicator_y = y;
                }

                // Mark indicator tile type
                if (spot->indicator_x >= 0 && spot->indicator_y >= 0)
                {
                    Tile *t1 = &map->tiles[spot->indicator_y][spot->indicator_x];
                    t1->type = TILE_PARKING_INDICATOR;
                    // Mark tile underneath as belonging to this spot
                    Tile *t2 = &map->tiles[spot->indicator_y + 1][spot->indicator_x];
                    t2->type = TILE_PARKING_INDICATOR;
                    // Link indicator tiles to this spot for renderer
                    t1->spot = spot;
                    t2->spot = spot;
                }

                map->parking_count++;
            }

            // Skip the remaining 7 tiles so we don't detect the same spot again
            x += 7;
        }
    }
}

const ParkingSpot *map_get_parking_spot_with_indicator(const Map *map, int x, int y)
{
    for (int i = 0; i < map->parking_count; ++i)
    {
        const ParkingSpot *s = &map->parkings[i];
        if (s->indicator_x == x && s->indicator_y == y)
            return s;
    }
    return NULL;
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

const Waypoint *map_get_waypoint_by_id(const Map *map, int id)
{
    for (int i = 0; i < map->waypoint_count; ++i)
    {
        if (map->waypoints[i].id == id)
        {
            return &map->waypoints[i];
        }
    }
    return NULL;
}

void map_debug_print_parking(const Map *map)
{
    printf("Parking spots (%d):\n", map->parking_count);
    for (int i = 0; i < map->parking_count; ++i)
    {
        const ParkingSpot *s = &map->parkings[i];
        printf("  Spot %d: cells=%d, anchor=(%d,%d), indicator=(%d,%d)\n",
               s->id,
               s->x0, s->y0,
               s->indicator_x, s->indicator_y);
    }
}
