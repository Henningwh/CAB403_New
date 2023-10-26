#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>
#include "ipLib.h"
#include "overseer.h"


int splitString(const char *input, char *delimiter, char *result[], int maxSegments) {
    char *str = strdup(input); // Duplicate the input string to avoid modification
    char *token = strtok(str, delimiter);

    int segmentCount = 0;

    while (token != NULL && segmentCount < maxSegments) {
        result[segmentCount] = strdup(token);
        token = strtok(NULL, delimiter);
        segmentCount++;
    }

    free(str); // Free the duplicated string

    return segmentCount;
}

/**
 * Handles cli inputs.
*/
void handleCommand(char* command){
}

char* moduleName = "Overseer"; 

void customHandleRecieveFromTestModule(char* msg, int remoteSocketFD){
    if(strcmp(msg, "Hi from test module") == 0){
        sendAndPrintFromModule(moduleName,"Hi back from overseer#", remoteSocketFD); 
        char* gotStuff = recieveAndPrintMsg(remoteSocketFD, moduleName);
        if(strcmp(gotStuff, "please send stuff") == 0){
            sendAndPrintFromModule(moduleName,"here is stuff#", remoteSocketFD);
        }else{
            printf("Overseer: Supposed to get 'please send stuff#'. Got: %s\n", gotStuff);
        }

    }else{
        printf("Overseer: Supposed to get 'Hi from test module#'. Got: %s\n", msg);
    }
}

void customHandleRecieveMessagesInOverseer(int remoteSocketFD){
    
    char* msg = recieveAndPrintMsg(remoteSocketFD, moduleName);
    //buffer[charCount] = 0;
    customHandleRecieveFromTestModule(msg, remoteSocketFD);
}



void overseer(){
    //char *arg[] = {"processName", "overseer", "127.0.0.1:3000", "2000", "250", "auth_file", "connection_file", "layout_file" ,"shm_path", "shm_offset"};
    char *argV[] = {"127.0.0.1:3000", "2000", "250", "auth_file", "connection_file", "layout_file" ,"shm_path", "shm_offset"};
    char **resultArray = (char **)malloc(10 * sizeof(char *));
    char *input = argV[0];
    int maxSeqments = 10;
    splitString(input, ":", resultArray, maxSeqments);
    char* address = strdup(resultArray[0]);
    int port = atoi(resultArray[1]);
    free(resultArray);

    printf("Address: %s, Port: %d\n", address, port);

    for(int i = 0; i<8; i++){
        printf("argV[%d]: %s\n", i, argV[i]);
    }

    //Ip stuff///////////////////////
    int listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in *listenAddr = createAddress(port, "");
    //lets the overseer accept all connections since no ip address was specified in listenAddr
    int bindRes = bind(listenSocketFD, listenAddr, sizeof(*listenAddr));
    if(bindRes == 0){
        printf("%s: Listen socket bound to port %d\n",moduleName, port);
    }else{
        printf("%s: Bind to port %d FAILED\n",moduleName, port);
    }
    struct CustomRecieveMsgHandlerAndDependencies overseerMsgHandlerStruct;
    overseerMsgHandlerStruct.customMsgHandler = customHandleRecieveMessagesInOverseer;
    overseerMsgHandlerStruct.listenSocketFD = listenSocketFD;
    overseerMsgHandlerStruct.moduleName = moduleName;

    //Continously listening for connections and queueing up to 100
    int connections = listen(listenSocketFD,100);
    printf("%s: started listening for incomming connections...\n", moduleName);
    //accepting connections and messages in separate thread
    pthread_t id;
    pthread_create(&id,NULL,continouslyAcceptConnections,(void*)&overseerMsgHandlerStruct);


    char *command = NULL;
    size_t commandLen= 0;
    printf("%s: Ready for cli commands:\n", moduleName);


    char buffer[1024];


    while(true){
        ssize_t  charCount = getline(&command,&commandLen,stdin);
        command[charCount-1] = 0;
        printf("command: %s \n", command);
        if(strcmp(command,"exit")==0){
            break;
        }
        handleCommand(command);


    }

}