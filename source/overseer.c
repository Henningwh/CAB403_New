#include<stdio.h>

void printStrings(char *strArray[]) {
    for (int i = 0; strArray[i] != NULL; i++) {
        printf("%s\n", strArray[i]);
    }
}

void overseer(int argC, char *argV[]){
    printf("inside overseer\n");
    /* for(int i = 0; i<5; i++){
        printf("argV[%d]: %s\n", i, argV[i]);
    } */
}