#ifndef IPLIBUDP_H
#define IPLIBUDP_H

#include "dg_structs.h"

typedef void (*CustomUDPMsgHandler)(char *);

struct RecieveUDPMsgStruct
{
    char *moduleName;
    int listenSocketFD;
    CustomUDPMsgHandler customParseAndHandleMessage;
};

extern int overseerUdpPort;

int openAndBindNewUdpPort(int port, char *moduleName);

void continouslyRecieveUDPMsgAndPrint(struct RecieveUDPMsgStruct *resStruct);

void sendUDPMessage(char *message, char *ip, int port, char *moduleN);
void sendUDPMessageTemp(temp_update_dg *message, char *ip, int port, char *moduleName);
void continouslyRecieveUDPMsgAndPrintTemp(struct RecieveUDPMsgStruct *resStruct);

#endif