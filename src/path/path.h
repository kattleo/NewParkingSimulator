#ifndef PATH_H
#define PATH_H

#include <stdbool.h>

#define MAX_PATH_STEPS 1024

struct Map;

typedef struct
{
    int x;
    int y;
} PathStep;

typedef struct
{
    PathStep steps[MAX_PATH_STEPS];
    int length;
} Path;

// Initialize a path (sets length to 0)
void path_init(Path *p);


// Find a shortest path from (sx, sy) to (gx, gy) for a car of given size and orientation.
// Returns true on success and fills out_path with the path.
// Only steps where the car's full footprint fits are allowed.
bool path_find_with_size(const struct Map *map,
                        int sx, int sy,
                        int gx, int gy,
                        int car_width, int car_height,
                        Path *out_path);

// Legacy: single-tile pathfinding (for non-cars)
bool path_find(const struct Map *map,
               int sx, int sy,
               int gx, int gy,
               Path *out_path);

#endif // PATH_H
