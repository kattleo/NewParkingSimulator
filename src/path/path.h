#ifndef PATH_H
#define PATH_H

#define MAX_PATH_STEPS 256

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

#endif
