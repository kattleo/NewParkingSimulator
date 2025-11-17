#ifndef RENDER_H
#define RENDER_H

#include "../map/map.h"
#include "../vehicle/vehicle.h"
#include "../vehicle/vehicle_list.h"

typedef struct
{
    int width;
    int height;
    char **buffer; // buffer[height][width]
} Screen;

// Allocate screen buffer based on map size
int screen_init(Screen *s, const Map *map);

// Free buffer
void screen_free(Screen *s);

// Fill buffer with static map glyphs
void screen_from_map(Screen *s, const Map *map);

// Overlay one vehicle (multi-tile sprite)
void screen_draw_vehicle(Screen *s, const Vehicle *v, const Map *map);

// Present buffer to terminal
void screen_present(const Screen *s, const Map *map, int step);

void screen_draw_paths(Screen *s, const VehicleList *vehicles);

#endif // RENDER_H
