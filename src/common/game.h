#ifndef GAME_H
#define GAME_H

typedef struct {
    int min_parking_time_smooth;
    int max_parking_time_smooth;
    int min_parking_time_busy;
    int max_parking_time_busy;
    int spawn_rate_smooth;
    int spawn_rate_busy;
    int frame_dt_ms_smooth;
    int frame_dt_ms_busy;
    int spawn_rate_ms; // selected mode
    int min_parking_time_sec; // selected mode
    int max_parking_time_sec; // selected mode
    int frame_dt_ms; // selected mode
    int show_intro;
    int debug_logs;
} Config;

// Load config from file (simple key = value, ignores comments)
void config_load(Config *cfg, const char *filename);

typedef struct {
    int account_balance;
} Game;

#endif // GAME_H
