#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

// Helper to get terminal width
static int get_term_width_logo() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col > 0 ? w.ws_col : 80;
}

void show_logo_animated() {
    FILE *f = fopen("assets/logo.txt", "r");
    if (!f) return;
    char *lines[32];
    int n = 0, maxlen = 0;
    char buf[512];
    while (fgets(buf, sizeof(buf), f) && n < 32) {
        size_t len = strlen(buf);
        if (buf[len-1] == '\n') buf[len-1] = 0, --len;
        lines[n] = strdup(buf);
        if ((int)len > maxlen) maxlen = len;
        n++;
    }
    fclose(f);
    int width = get_term_width_logo();
    int pad_left = (width - maxlen) / 2;
    if (pad_left < 0) pad_left = 0;
    for (int col = 1; col <= maxlen; ++col) {
        printf("\033[2J\033[H"); // clear screen
        int term_height = 24;
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) term_height = w.ws_row;
        int pad_top = (term_height - n) / 2;
        if (pad_top < 0) pad_top = 0;
        for (int k = 0; k < pad_top; ++k) putchar('\n');
        for (int i = 0; i < n; ++i) {
            int line_len = strlen(lines[i]);
            int effective_col = col < line_len ? col : line_len;
            int pad = pad_left + (maxlen - line_len) / 2;
            if (pad < 0) pad = 0;
            printf("%*s", pad, "");
            for (int j = 0; j < effective_col; ++j)
                putchar(lines[i][j]);
            putchar('\n');
        }
        fflush(stdout);
        usleep(12000); // 12ms per column
    }
    // Print full logo at the end
    printf("\033[2J\033[H");
    int term_height = 24;
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) term_height = w.ws_row;
    int pad_top = (term_height - n) / 2;
    if (pad_top < 0) pad_top = 0;
    for (int k = 0; k < pad_top; ++k) putchar('\n');
    for (int i = 0; i < n; ++i) {
        int line_len = strlen(lines[i]);
        int pad = pad_left + (maxlen - line_len) / 2;
        if (pad < 0) pad = 0;
        printf("%*s%s\n", pad, "", lines[i]);
        free(lines[i]);
    }
    fflush(stdout);
    usleep(1000000); // Wait 1 second after full logo
}
