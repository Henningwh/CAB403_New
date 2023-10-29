#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>
#include "ipLibUDP.h"

#define MAX_BUFFER_SIZE 1024
int overseerUdpPort = 5000;

/**
 * Accepts all incomming addresses if no address is specified
 * Binds the port specified to this process
*/
struct sockaddr_in* createUDPAddress(int port, char *ip) {
    struct sockaddr_in  *address = malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
  address->sin_port = htons(port);
  address->sin_addr.s_addr = inet_addr(ip);
    return address;
}

int openAndBindNewUdpPort(int port, char* moduleName){
    printf("%s: Opening new UDP port on port: %d...\n", moduleName, port);
    int sockFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFD < 0){
        perror("Could not create UDP-socket");
        exit(1);
    }

    struct sockaddr_in *addr = createUDPAddress(port, "127.0.0.1");
    int bindRes = bind(sockFD, addr, sizeof(*addr));
    if(bindRes == 0){
        printf("%s: Listen UDP-socket bound to port %d\n",moduleName, port);
    }else{
        printf("%s: Bind to UDP-port %d FAILED\n",moduleName, port);
        exit(1);
    }
    return sockFD;
}

void continouslyRecieveUDPMsgAndPrint(struct RecieveUDPMsgStruct* resStruct){
    while(1){
        struct sockaddr_in senderAddr;
        socklen_t senderAddrLen = sizeof(senderAddr);
        char buffer[MAX_BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        ssize_t bytesReceived = recvfrom(resStruct->listenSocketFD, buffer, sizeof(buffer), 0, (struct sockaddr*)&senderAddr, &senderAddrLen);
        if (bytesReceived == -1) {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        //buffer[bytesReceived] = '\0'; // Null-terminate the received data

        char* message = strdup(buffer); // Create a copy of the received message

        printf("%s: Received UDP message from %s:%d: %s\n", resStruct->moduleName, inet_ntoa(senderAddr.sin_addr),ntohs(senderAddr.sin_port), message);

        pthread_t id;
        pthread_create(&id,NULL,resStruct->customParseAndHandleMessage, message);
    }
}

void sendUDPMessage(char* message, char* ip, int port, char* moduleName) {
    int localSocket;

    // Create a UDP socket
    localSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (localSocket == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in *remoteAddr = createUDPAddress(port, ip);

    // Send the message to the server

    //sendto(sockfd, buffer, 1024, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));

    ssize_t bytesSent = sendto(localSocket, message, 1024, 0, remoteAddr, sizeof(*remoteAddr));
    if (bytesSent == -1) {
        perror("sendto");
        close(localSocket);
        exit(1);
    }

    printf("%s: Sent UDP message to %s:%d: %s\n",moduleName, ip, port, message);

    close(localSocket);
}