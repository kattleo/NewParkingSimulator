#include "path.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../map/map.h"

// Macros to convert between (x,y) and flat index
#define IDX(x, y, width) ((y) * (width) + (x))

void path_init(Path *p)
{
    p->length = 0;
}

bool path_find(const struct Map *map,
               int sx, int sy,
               int gx, int gy,
               Path *out_path)
{
    path_init(out_path);

    int width = map->width;
    int height = map->height;
    int num_cells = width * height;

    // Check if start/goal inside map and walkable
    if (sx < 0 || sx >= width || sy < 0 || sy >= height)
        return false;
    if (gx < 0 || gx >= width || gy < 0 || gy >= height)
        return false;

    if (!map_is_walkable(map, sx, sy))
        return false;
    if (!map_is_walkable(map, gx, gy))
        return false;

    int start_idx = IDX(sx, sy, width);
    int goal_idx = IDX(gx, gy, width);

    // Trivial case: start == goal
    if (start_idx == goal_idx)
    {
        out_path->length = 1;
        out_path->steps[0].x = sx;
        out_path->steps[0].y = sy;
        return true;
    }

    // Allocate BFS buffers
    unsigned char *visited = calloc(num_cells, sizeof(unsigned char));
    int *came_from = malloc(num_cells * sizeof(int));
    int *queue = malloc(num_cells * sizeof(int));

    if (!visited || !came_from || !queue)
    {
        free(visited);
        free(came_from);
        free(queue);
        return false;
    }

    // Initialize came_from with sentinel -1
    for (int i = 0; i < num_cells; ++i)
    {
        came_from[i] = -1;
    }

    // BFS setup
    int head = 0;
    int tail = 0;

    queue[tail++] = start_idx;
    visited[start_idx] = 1;

    // 4 neighbors: E, W, N, S
    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, -1, 1};

    int found = 0;

    // BFS loop
    while (head < tail)
    {
        int current = queue[head++];
        if (current == goal_idx)
        {
            found = 1;
            break;
        }

        int cx = current % width;
        int cy = current / width;

        for (int dir = 0; dir < 4; ++dir)
        {
            int nx = cx + dx[dir];
            int ny = cy + dy[dir];

            if (nx < 0 || nx >= width || ny < 0 || ny >= height)
                continue;

            int n_idx = IDX(nx, ny, width);

            if (visited[n_idx])
                continue;

            if (!map_is_walkable(map, nx, ny))
                continue;

            visited[n_idx] = 1;
            came_from[n_idx] = current;
            queue[tail++] = n_idx;
        }
    }

    if (!found)
    {
        // No path
        free(visited);
        free(came_from);
        free(queue);
        return false;
    }

    // Reconstruct path backwards from goal_idx
    int path_buf_len = 0;
    int cur = goal_idx;

    while (cur != -1 && cur != start_idx)
    {
        int x = cur % width;
        int y = cur / width;

        if (path_buf_len >= MAX_PATH_STEPS)
        {
            // Path too long for buffer
            free(visited);
            free(came_from);
            free(queue);
            return false;
        }

        out_path->steps[path_buf_len].x = x;
        out_path->steps[path_buf_len].y = y;
        path_buf_len++;

        cur = came_from[cur];
    }

    // Add start tile
    if (path_buf_len >= MAX_PATH_STEPS)
    {
        free(visited);
        free(came_from);
        free(queue);
        return false;
    }
    out_path->steps[path_buf_len].x = sx;
    out_path->steps[path_buf_len].y = sy;
    path_buf_len++;

    // Now steps are [goal, ..., start] -> reverse to [start, ..., goal]
    for (int i = 0; i < path_buf_len / 2; ++i)
    {
        PathStep tmp = out_path->steps[i];
        out_path->steps[i] = out_path->steps[path_buf_len - 1 - i];
        out_path->steps[path_buf_len - 1 - i] = tmp;
    }

    out_path->length = path_buf_len;

    free(visited);
    free(came_from);
    free(queue);
    return true;
}