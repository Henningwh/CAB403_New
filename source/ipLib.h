#ifndef IPLIB_H
#define IPLIB_H

#include <arpa/inet.h>

typedef void (*CustomMsgHandler)(int);

struct CustomRecieveMsgHandlerAndDependencies {
    CustomMsgHandler customMsgHandler;
    char* moduleName;
    int listenSocketFD;
};

struct CustomSendMsgHandlerAndDependencies {
    CustomMsgHandler customMsgHandler;
    int remotePort;
    char* remoteAddr;
    char* moduleName;
    char* msg;
};

struct ConnectedRemoteSocket {
    int remoteSocketFD;
    struct sockaddr_in remoteAddr;
};

void sendAndPrintFromModule(char* moduleName, char* msg, int localSockFD);

char* recieveAndPrintMsg(int remoteSocketFD, char* moduleName);

struct sockaddr_in* createAddress(int port, char* ipAddr);

void continouslyAcceptConnections(struct CustomRecieveMsgHandlerAndDependencies* msgHandlerAndDeps);

void connectToRemoteSocketAndSendMessage(struct CustomSendMsgHandlerAndDependencies* msgHandlerAndDeps);

#endif
