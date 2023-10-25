#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

int splitString(const char *input, char *delimiter, char *result[], int maxSegments) {
    char *str = strdup(input);
    char *token = strtok(str, delimiter);
    int segmentCount = 0;

    while (token != NULL && segmentCount < maxSegments) {
        result[segmentCount] = strdup(token);
        token = strtok(NULL, delimiter);
        segmentCount++;
    }

    free(str);
    return segmentCount;
}

struct ConnectedRemoteSocket {
    int remoteSocketFD;
    struct sockaddr_in remoteAddr;
};

struct ConnectedRemoteSocket connectedSockets[100];
int connectedSocketsCount = 0;

struct sockaddr_in* createListenAddress(int port, char* ipAddr) {
    struct sockaddr_in *listenAddr = malloc(sizeof(struct sockaddr_in));
    listenAddr->sin_family = AF_INET;
    listenAddr->sin_port = htons(port);
    if (strcmp(ipAddr, "") == 0) {
        listenAddr->sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, ipAddr, &listenAddr->sin_addr.s_addr);
    }
    return listenAddr;
}

void* handleClientMessages(void *arg) {
    struct ConnectedRemoteSocket *remoteSocket = (struct ConnectedRemoteSocket *)arg;
    char buffer[BUFFER_SIZE];
    int bytesRead;

    while (1) {
        bytesRead = recv(remoteSocket->remoteSocketFD, buffer, BUFFER_SIZE, 0);
        if (bytesRead < 0) {
            perror("Error receiving data");
            close(remoteSocket->remoteSocketFD);
            return NULL;
        } else if (bytesRead == 0) {
            printf("Connection closed by the remote client.\n");
            close(remoteSocket->remoteSocketFD);
            return NULL;
        } else {
            buffer[bytesRead] = '\0'; // Null-terminate the received string
            printf("Received message: %s\n", buffer);

            // Add your message processing logic here if needed

            // Sending a simple acknowledgment (you can expand on this)
            char response[] = "Received your message!";
            send(remoteSocket->remoteSocketFD, response, sizeof(response), 0);
        }
    }
}

void spawnThreadsAndHandleMessages(struct ConnectedRemoteSocket* remoteSocket) {
    pthread_t threadID;
    pthread_create(&threadID, NULL, handleClientMessages, (void *)remoteSocket);
    pthread_detach(threadID); // So we don't need to call pthread_join()
}

void continouslyAcceptConnections(int listenSocketFD) {
    while (1) {
        struct sockaddr_in remoteAddr;
        socklen_t remoteAddrSize = sizeof(remoteAddr);
        int remoteSocketFD = accept(listenSocketFD, (struct sockaddr*) &remoteAddr, &remoteAddrSize);
        if (remoteSocketFD > 0) {
            printf("Overseer: Socket with port: %d connected successfully\n", ntohs(remoteAddr.sin_port));
        } else {
            perror("Overseer: Connection failed when accepting connection from socket");
            continue;
        }

        struct ConnectedRemoteSocket remoteSocket;
        remoteSocket.remoteAddr = remoteAddr;
        remoteSocket.remoteSocketFD = remoteSocketFD;
        connectedSockets[connectedSocketsCount++] = remoteSocket;
        spawnThreadsAndHandleMessages(&remoteSocket);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ip:port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char **resultArray = (char **)malloc(2 * sizeof(char *));
    int maxSegments = 2;
    splitString(argv[1], ":", resultArray, maxSegments);
    char* address = resultArray[0];
    int port = atoi(resultArray[1]);

    int listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocketFD < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in *listenAddr = createListenAddress(port, address);
    if (bind(listenSocketFD, (struct sockaddr*)listenAddr, sizeof(*listenAddr)) < 0) {
        perror("Bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(listenSocketFD, 100) < 0) {
        perror("Listen error");
        exit(EXIT_FAILURE);
    }

    continouslyAcceptConnections(listenSocketFD);
    
    free(resultArray);
    free(listenAddr);
    return 0;
}
