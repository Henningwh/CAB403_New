#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>
#include "ipLib.h"

struct ConnectedRemoteSocket connectedSockets[100];
int connectedSocketsCount = 0;

void sendAndPrintFromModule(char* moduleName, char* msg, int localSockFD){
    int sentCount = send(localSockFD, msg , 1024, 0);
    if(sentCount==0){
        printf("%s: Could not send message, remote socket closed connection: %s\n", moduleName, msg);
    }else{
        printf("%s: Successfully sent: %s\n", moduleName, msg);
    }
    //else{
    //    printf("%s: Error when sending message: %s\n", moduleName, msg);
    //}

}


char* recieveAndPrintMsg(int socketFD, char* moduleName){
    //printf("%s: socketID: %d\n", moduleName, socketFD);
    char receivedChar;
    char* receivedMessage = NULL;
    size_t messageSize = 0;

    while (1) {
        ssize_t bytesRead = recv(socketFD, &receivedChar, 1, 0);
        //printf("%s: %c\n", moduleName, receivedChar);
        if (bytesRead == -1) {
            perror("recv");
            free(receivedMessage);
            return NULL;
        } else if (bytesRead == 0) {
            // Connection closed
            break;
        }
        if(receivedChar == '\0'){
            //printf("This is indeed the problem\n");
            receivedMessage = NULL;
            messageSize = 0;

            continue;
        }

        if (receivedChar == '#') {
            // End of message, break the loop
            break;
        }

        // Reallocate memory for the received message and append the character
        char* newMessage = (char*)realloc(receivedMessage, messageSize + 2); // +2 for the new character and null terminator
        if (newMessage == NULL) {
            perror("realloc");
            free(receivedMessage);
            return NULL;
        }
        receivedMessage = newMessage;

        receivedMessage[messageSize] = receivedChar;
        receivedMessage[messageSize + 1] = '\0';
        messageSize++;
    }

    // Print the received message
    printf("%s: Received Message: %s\n", moduleName, receivedMessage);

    return receivedMessage;
}

/**
 * Accepts all incomming addresses if no address is specified
 * Binds the port specified to this process
*/
struct sockaddr_in* createAddress(int port, char *ip) {
    struct sockaddr_in  *address = malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    if(strlen(ip) ==0){
        address->sin_addr.s_addr = INADDR_ANY;
        }else{
            inet_pton(AF_INET,ip,&address->sin_addr.s_addr);
        }
    return address;
}

struct sockaddr_in* createConnectAddress(int port, char* ipAddr){
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

void connectToRemoteSocketAndSendMessage(struct CustomSendMsgHandlerAndDependencies* msgHandlerAndDeps){
    //Creating the local socket that will connect to the remote socket. This is a temporary socket.
    int localSocket = socket(AF_INET, SOCK_STREAM, 0);
    int rp = msgHandlerAndDeps->remotePort;
    char* addraddr = msgHandlerAndDeps->remoteAddr;
    struct sockaddr_in *remoteAddr = createAddress(msgHandlerAndDeps->remotePort, "");
    int connectionRes = connect(localSocket,remoteAddr, sizeof(*remoteAddr));
    //wait(1);
    if (connectionRes == 0){
        printf("%s: connected successfully to %s:%d\n",msgHandlerAndDeps->moduleName, msgHandlerAndDeps->remoteAddr, msgHandlerAndDeps->remotePort);
    } else{
        printf("%s: Connection error. Could not connect to %s:%d\n",msgHandlerAndDeps->moduleName, msgHandlerAndDeps->remoteAddr, msgHandlerAndDeps->remotePort);
    }
    msgHandlerAndDeps->customMsgHandler(localSocket);
    //close(localSocket);
    //printf("%s: Closed local socket\n", msgHandlerAndDeps->moduleName);
}

void continouslyAcceptConnections(struct CustomRecieveMsgHandlerAndDependencies* msgHandlerAndDeps){
    while(true){
        struct sockaddr_in  remoteAddr ;
        int remoteAddrSize = sizeof (struct sockaddr_in);
        //Gives the file descriptor id of the remote socket
        int remoteSocketFD = accept(msgHandlerAndDeps->listenSocketFD, &remoteAddr, &remoteAddrSize);
        if(remoteSocketFD > 0){
            printf("%s: Socket with port: %d connected successfully\n",msgHandlerAndDeps->moduleName, remoteAddr.sin_port);
        }else{
            printf("%s: Connection failed when accepting connection from socket\n", msgHandlerAndDeps);
        }
        struct ConnectedRemoteSocket* remoteSocket = malloc(sizeof (struct ConnectedRemoteSocket));
        remoteSocket->remoteAddr = remoteAddr;
        remoteSocket->remoteSocketFD = remoteSocketFD;
        //Adding the connected sockets to an array. Might need to separate arrays for different modules
        //connectedSockets[connectedSocketsCount++] = *remoteSocket;

        //ToDo: deal with responses to the recieved messages
        //spawnThreadsAndHandleMessages(remoteSocket);
        pthread_t id;
        //printf("Overseer: tries to create msgRecieveHandler thread\n");
        pthread_create(&id,NULL,msgHandlerAndDeps->customMsgHandler, remoteSocketFD);
        //printf("Overseer: spawned msgRecieveHandler thread\n");
        //msgHandlerAndDeps->customMsgHandler(remoteSocketFD);

    }
}

