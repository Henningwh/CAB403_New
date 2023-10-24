#include <stdio.h>
#include "door.c"
#include "overseer.c"

int main(int argC, char *argV[]){
    char *arg[] = {"processName", "overseer", "127.0.0.1:3000", "2000", "250", "auth_file", "connection_file", "layout_file" ,"shm_path", "shm_offset"};
    if(arg[1] == "overseer"){
        overseer(7, &arg[2]);
    }

    if(argV[1] == "door"){
        door(argC-2, &argV[2]);
    }

    return 0;
}

