#include <stdio.h>

void door(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s {address:port}\n", argv[0]);
        return;
    }
    printf("Door is running on %s\n", argv[1]);
}