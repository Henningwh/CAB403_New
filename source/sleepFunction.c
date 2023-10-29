#define _XOPEN_SOURCE 700
#include <unistd.h>

/*
* Needs its own file because we define _XOPEN_SOURCE 700
*/

void sleepMicroseconds(unsigned int microseconds) {
    usleep(microseconds);
}