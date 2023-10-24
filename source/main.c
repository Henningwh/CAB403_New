#include <stdio.h>
#include "door.c"
#include "overseer.c"

int main(int argC, char *argV[]){
    char *arg[] = {"processName", "overseer", "127.0.0.1:3001", "FAIL_SAFE", "shm_path", "shm_offset", "127.0.0.1:3000", NULL};
    printStrings(&arg[2]);
    if(arg[1] == "overseer"){
        overseer(argC-2, arg[2]);
    }

}