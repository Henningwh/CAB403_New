#include <stdio.h>
#include <pthread.h>
#include <unistd.h> 
//#include "door.c"
#include "overseer.h"
#include "simpleTestModule.h"

int main(int argC, char *argV[]){
    char *arg[] = {"processName", "overseer", "127.0.0.1:3000", "2000", "250", "auth_file", "connection_file", "layout_file" ,"shm_path", "shm_offset"};
    if(arg[1] == "overseer"){
        pthread_t overseerID;
        pthread_create(&overseerID, NULL, overseer, NULL);
        sleep(1);
        pthread_t testModID;
        pthread_create(&testModID, NULL, runTestModule, NULL);

        pthread_join(overseerID, NULL);
        pthread_join(testModID, NULL);
    }

    if(argV[1] == "door"){
//        door(argC-2, &argV[2]);
    }

    return 0;
}

