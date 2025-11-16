#ifndef PATH_H
#define PATH_H

#include <stdbool.h>
#include "../map/map.h"

typedef struct
{
    int x;
    int y;
} PathStep;

#define MAX_PATH_STEPS 1024

typedef struct
{
    PathStep steps[MAX_PATH_STEPS];
    int length;
} Path;

// Initialize a path (sets length to 0)
void path_init(Path *p);

// Find a shortest path from (sx, sy) to (gx, gy) on the given map using BFS.
// Returns true on success and fills out_path with the path
// (steps[0] = start, steps[length-1] = goal).
// Returns false if no path found or if it would exceed MAX_PATH_STEPS.
bool path_find(const Map *map,
               int sx, int sy,
               int gx, int gy,
               Path *out_path);

#endif // PATH_H
