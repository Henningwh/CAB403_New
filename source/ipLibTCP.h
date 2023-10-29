#ifndef IPLIB_H
#define IPLIB_H

#include <arpa/inet.h>

struct CustomMsgHandlerArgs {
    int socket;
    char** arguments;
};

typedef void (*CustomMsgHandler)(struct CustomMsgHandlerArgs*);

struct CustomRecieveMsgHandlerAndDependencies {
    CustomMsgHandler customMsgHandler;
    char* moduleName;
    int listenSocketFD;
    char** arguments;
};


struct CustomSendMsgHandlerAndDependencies {
    CustomMsgHandler customMsgHandler;
    int remotePort;
    char* remoteAddr;
    char* moduleName;
    char** arguments;
};

struct ConnectedRemoteSocket {
    int remoteSocketFD;
    struct sockaddr_in remoteAddr;
};

int openAndBindNewTCPport(int port, char* moduleName);

void sendAndPrintFromModule(char* moduleName, char* msg, int localSockFD);

char* recieveAndPrintMsg(int remoteSocketFD, char* moduleName);

struct sockaddr_in* createAddress(int port, char* ipAddr);

void continouslyAcceptConnections(struct CustomRecieveMsgHandlerAndDependencies* msgHandlerAndDeps);

void connectToRemoteSocketAndSendMessage(struct CustomSendMsgHandlerAndDependencies* msgHandlerAndDeps);

#endif
