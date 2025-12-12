#include "game.h"
#include <stdio.h>
#include <string.h>

void config_load(Config *cfg, const char *filename) {
    // Set defaults
    cfg->min_parking_time_smooth = 3;
    cfg->max_parking_time_smooth = 10;
    cfg->min_parking_time_busy = 1;
    cfg->max_parking_time_busy = 4;
    cfg->spawn_rate_smooth = 2000;
    cfg->spawn_rate_busy = 600;
    cfg->frame_dt_ms_smooth = 150;
    cfg->frame_dt_ms_busy = 60;
    cfg->show_intro = 1;
    cfg->debug_logs = 1;
    FILE *f = fopen(filename, "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || strlen(line) < 3) continue;
        char key[64]; int val;
        if (sscanf(line, "%63[^=]=%d", key, &val) == 2) {
            char *p = key;
            while (*p == ' ' || *p == '\t') ++p;
            if (strstr(p, "min_parking_time_smooth")) cfg->min_parking_time_smooth = val;
            else if (strstr(p, "max_parking_time_smooth")) cfg->max_parking_time_smooth = val;
            else if (strstr(p, "min_parking_time_busy")) cfg->min_parking_time_busy = val;
            else if (strstr(p, "max_parking_time_busy")) cfg->max_parking_time_busy = val;
            else if (strstr(p, "spawn_rate_smooth")) cfg->spawn_rate_smooth = val;
            else if (strstr(p, "spawn_rate_busy")) cfg->spawn_rate_busy = val;
            else if (strstr(p, "frame_dt_ms_smooth")) cfg->frame_dt_ms_smooth = val;
            else if (strstr(p, "frame_dt_ms_busy")) cfg->frame_dt_ms_busy = val;
            else if (strstr(p, "show_intro")) cfg->show_intro = val;
            else if (strstr(p, "debug_logs")) cfg->debug_logs = val;
        }
    }
    fclose(f);
}
