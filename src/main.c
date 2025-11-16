#include <stdio.h>
#include "map/map.h"

int main(void)
{
    Map map;
    if (!map_load(&map, "assets/map.txt"))
    {
        return 1;
    }

    // Print the map
    map_print(&map);

    map_free(&map);
    return 0;
}
