// Macros to convert between (x,y) and flat index
#define IDX(x, y, width) ((y) * (width) + (x))

#include <stdlib.h>
#include <stdbool.h>
#include "../vehicle/vehicle.h"
#include <stdio.h>
// Helper: check if car of size (w,h) fits at (x,y) on map
static int car_fits_at(const struct Map *map, int x, int y, int w, int h) {
    for (int dy = 0; dy < h; ++dy) {
        for (int dx = 0; dx < w; ++dx) {
            int tx = x + dx;
            int ty = y + dy;
            if (!map_is_walkable(map, tx, ty)) {
                return 0;
            }
        }
    }
    return 1;
}

bool path_find_with_size(const struct Map *map,
                        int sx, int sy,
                        int gx, int gy,
                        int car_width, int car_height,
                        Path *out_path)
{
    path_init(out_path);

    int width = map->width;
    int height = map->height;
    int num_cells = width * height;

    // Check if start/goal inside map and car fits
    if (sx < 0 || sx >= width || sy < 0 || sy >= height)
        return false;
    if (gx < 0 || gx >= width || gy < 0 || gy >= height)
        return false;

    if (!car_fits_at(map, sx, sy, car_width, car_height))
        return false;
    if (!car_fits_at(map, gx, gy, car_width, car_height))
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


    // --- Path finding implementation ---
    typedef struct {
        int idx;
        int f; // total cost = g + h
    } HeapNode;
    // Binary min-heap for up to num_cells
    HeapNode *heap = malloc(num_cells * sizeof(HeapNode));
    int heap_size = 0;
    int *g_score = malloc(num_cells * sizeof(int));
    int *came_from = malloc(num_cells * sizeof(int));
    unsigned char *visited = calloc(num_cells, sizeof(unsigned char));
    if (!heap || !g_score || !came_from || !visited) {
        free(heap); free(g_score); free(came_from); free(visited);
        return false;
    }
    for (int i = 0; i < num_cells; ++i) {
        g_score[i] = 99999999;
        came_from[i] = -1;
    }
    g_score[start_idx] = 0;
    // Heap push
    heap[heap_size++] = (HeapNode){start_idx, abs(sx-gx)+abs(sy-gy)};
    visited[start_idx] = 1;
    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, -1, 1};
    int found = 0;
    while (heap_size > 0) {
        // Pop min-f node
        int min_idx = 0;
        for (int i = 1; i < heap_size; ++i) {
            if (heap[i].f < heap[min_idx].f) min_idx = i;
        }
        HeapNode node = heap[min_idx];
        heap[min_idx] = heap[--heap_size];
        int current = node.idx;
        if (current == goal_idx) { found = 1; break; }
        int cx = current % width;
        int cy = current / width;
        for (int dir = 0; dir < 4; ++dir) {
            int nx = cx + dx[dir];
            int ny = cy + dy[dir];
            if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
            int n_idx = IDX(nx, ny, width);
            if (!car_fits_at(map, nx, ny, car_width, car_height)) continue;
            int tentative_g = g_score[current] + 1;
            if (tentative_g < g_score[n_idx]) {
                g_score[n_idx] = tentative_g;
                came_from[n_idx] = current;
                int h = abs(nx-gx) + abs(ny-gy);
                int f = tentative_g + h;
                // Only add to heap if not already visited or better path found
                int already_in_heap = 0;
                for (int i = 0; i < heap_size; ++i) if (heap[i].idx == n_idx) { already_in_heap = 1; if (heap[i].f > f) heap[i].f = f; break; }
                if (!already_in_heap) {
                    heap[heap_size++] = (HeapNode){n_idx, f};
                }
            }
        }
    }
    free(g_score);
    free(heap);
    if (!found) {
        free(visited); free(came_from);
        return false;
    }

    int path_buf_len = 0;
    int cur = goal_idx;
    while (cur != -1 && cur != start_idx)
    {
        int x = cur % width;
        int y = cur / width;
        if (path_buf_len >= MAX_PATH_STEPS)
        {
            free(visited);
            free(came_from);
            return false;
        }
        out_path->steps[path_buf_len].x = x;
        out_path->steps[path_buf_len].y = y;
        path_buf_len++;
        cur = came_from[cur];
    }
    if (path_buf_len >= MAX_PATH_STEPS)
    {
        free(visited);
        free(came_from);
        return false;
    }
    out_path->steps[path_buf_len].x = sx;
    out_path->steps[path_buf_len].y = sy;
    path_buf_len++;
    for (int i = 0; i < path_buf_len / 2; ++i)
    {
        PathStep tmp = out_path->steps[i];
        out_path->steps[i] = out_path->steps[path_buf_len - 1 - i];
        out_path->steps[path_buf_len - 1 - i] = tmp;
    }
    out_path->length = path_buf_len;
    free(visited);
    free(came_from);
    return true;
}
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
    return true;
}