#include <stdio.h>
#include "door.c"

int main(int argC, char *argV[]){
    if(argV[1] == "door"){
        door(argC-2, argV[2]);
    }
    printf("heyhey");

    return 0;
}
