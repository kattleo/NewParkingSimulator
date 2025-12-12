#include "menu.h"
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

// Helper to get a single char from stdin without echo
static int getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// Helper to clear screen
static void clear_screen() {
    printf("\033[2J\033[H");
}

// Helper to get terminal width
static int get_term_width() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) return w.ws_col;
    return 80;
}

int menu_show() {
    const char *modes[] = {"Smooth", "Busy"};
    int selected = 0;
    int n_modes = 2;
    int running = 1;
    while (running) {
        clear_screen();
        int width = get_term_width();
        const char *title = "Choose game mode";
        int title_pad = (width - (int)strlen(title)) / 2;
        for (int i = 0; i < title_pad; ++i) putchar(' ');
        printf("%s\n\n", title);
        for (int i = 0; i < n_modes; ++i) {
            int pad = (width - (int)strlen(modes[i]) - 4) / 2;
            for (int j = 0; j < pad; ++j) putchar(' ');
            if (i == selected) printf("> %s <\n", modes[i]);
            else printf("  %s  \n", modes[i]);
        }
        printf("\nUse \033[34m↑\033[0m and \033[34m↓\033[0m arrows. Press Enter to start.");
        fflush(stdout);
        int ch = getch();
        if (ch == 27) { // Escape sequence
            if (getch() == '[') {
                ch = getch();
                if (ch == 'A') { // Up arrow
                    selected = (selected - 1 + n_modes) % n_modes;
                } else if (ch == 'B') { // Down arrow
                    selected = (selected + 1) % n_modes;
                }
            }
        } else if (ch == '\n' || ch == '\r') {
            running = 0;
        }
    }
    clear_screen();
    return selected;
}
