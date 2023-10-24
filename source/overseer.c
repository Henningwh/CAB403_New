#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>

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

struct ConnectedRemoteSocket
{
    int remoteSocketFD;
    struct sockaddr_in remoteAddr;
};

struct ConnectedRemoteSocket connectedSockets[100];
int connectedSocketsCount = 0;

/**
 * Accepts all incomming addresses if no address is specified
 * Binds the port specified to this process
*/
struct sockaddr_in* createListenAddress(int port, char* ipAddr){
    struct sockaddr_in  *listenAddr = malloc(sizeof(struct sockaddr_in));
    listenAddr->sin_family = AF_INET;
    listenAddr->sin_port = htons(port);
    if(listenAddr == ""){
        listenAddr->sin_addr.s_addr = INADDR_ANY;
    }else{
        inet_pton(AF_INET,ipAddr,&listenAddr->sin_addr.s_addr);
        }

    return listenAddr;
}

spawnThreadsAndHandleMessages(struct ConnectedRemoteSocket* remoteSocket){};


void continouslyAcceptConnections(int listenSocketFD){
    while(true){

        struct sockaddr_in  remoteAddr ;
        int remoteAddrSize = sizeof (struct sockaddr_in);
        //Gives the file descriptor id of the remote socket
        int remoteSocketFD = accept(listenSocketFD, &remoteAddr, &remoteAddrSize);
        if(remoteSocketFD > 0){
            printf("Overseer: Socket with port: %d connected successfully\n", remoteAddr.sin_port);
        }else{
            printf("Overseer: Connection failed when accepting connection from socket");
        }
        struct ConnectedRemoteSocket* remoteSocket = malloc(sizeof (struct ConnectedRemoteSocket));
        remoteSocket->remoteAddr = remoteAddr;
        remoteSocket->remoteSocketFD = remoteSocketFD;
        //Adding the connected sockets to an array. Might need to separate arrays for different modules
        connectedSockets[connectedSocketsCount++] = *remoteSocket;

        //ToDo: deal with responses to the recieved messages
        spawnThreadsAndHandleMessages(remoteSocket);

    }
}



void overseer(int argC, char *argV[]){
    printf("inside overseer\n");

    char **resultArray = (char **)malloc(10 * sizeof(char *));
    char *input = argV[0];
    int maxSeqments = 10;
    splitString(input, ":", resultArray, maxSeqments);
    char* address = strdup(resultArray[0]);
    int port = atoi(resultArray[1]);
    free(resultArray);

    printf("Address: %s, Port: %d\n", address, port);

    for(int i = 0; i<argC; i++){
        printf("argV[%d]: %s\n", i, argV[i]);
    }

    //Ip stuff///////////////////////
    int listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in* listenAddr = createListenAddress(port, "");
    //lets the overseer accept all connections since no ip address was specified in listenAddr
    int bindRes = bind(listenSocketFD, listenAddr, sizeof(*listenAddr));
    if(bindRes == 0){
        printf("Overseer: Listen socket bound to port %d\n", port);
    }else{
        printf("Overseer: Bind to port %d FAILED\n", port);
    }
    //Continously listening for connections and queueing up to 100
    int connections = listen(listenSocketFD,100);
    //accepting connections and messages in separate threads
    continouslyAcceptConnections(listenSocketFD);

}