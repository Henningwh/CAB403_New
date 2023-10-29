#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "ipLibTCP.h"
#include "data_structs.h"
#include "helper_functions.h"

char* moduleName = "Cardscanner";

char scanned[16];
pthread_mutex_t mutex;
pthread_cond_t scanned_cond;

char response; // 'Y' or 'N' (or '\0' at first)
pthread_cond_t response_cond;

//CARDREADER 101 SCANNED 0b9adf9c81fb959#
void customSendScanToOverseer(struct CustomMsgHandlerArgs* sockAndargs){
    char* id = sockAndargs->arguments[1];
    char msg[50];
    sprintf(msg, "CARDREADER %s SCANNED %s#", id, scanned);
    sendAndPrintFromModule(moduleName, msg, sockAndargs->socket);
    close(sockAndargs->socket);
}

//CARDREADER {id} HELLO#
void customSendHelloToOverseer(struct CustomMsgHandlerArgs* sockAndargs){
    char* id = sockAndargs->arguments[1];
    char msg[50];
    sprintf(msg, "CARDREADER %s HELLO#", id);
    sendAndPrintFromModule(moduleName, msg, sockAndargs->socket);
    close(sockAndargs->socket);
}

void cardreader(int argc, char*argv[]){

    char **resultArrayO = (char **)malloc(10 * sizeof(char *));
    char *inputO = argv[5];
    splitString(inputO, ":", resultArrayO, 10);
    char* addressO = strdup(resultArrayO[0]);
    int portO = atoi(resultArrayO[1]);
    free(resultArrayO);

    struct CustomSendMsgHandlerAndDependencies sendHelloStruct;
    sendHelloStruct.customMsgHandler = customSendHelloToOverseer;
    sendHelloStruct.remotePort = portO;
    sendHelloStruct.remoteAddr = addressO;
    sendHelloStruct.moduleName = moduleName;
    sendHelloStruct.arguments = argv;

    connectToRemoteSocketAndSendMessage((void*)&sendHelloStruct);

    // Shared memory setup
    char* shm_path = argv[3];
    int offset = atoi(argv[4]);

    void *base = open_shared_memory(shm_path);
    shm_cardreader *p = (shm_cardreader *)((char *)base + offset);
    //scanned = p->scanned;
    mutex = p->mutex;
    scanned_cond = p->scanned_cond;
    response = p->response;
    response_cond = p->response_cond;



    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&scanned_cond, NULL);
    pthread_cond_init(&response_cond, NULL);

    while(1){

        if(strcmp(scanned, "\0") == 0){
        pthread_cond_wait(&scanned_cond, &mutex);
        memcpy(scanned, p->scanned, sizeof(scanned));

        struct CustomSendMsgHandlerAndDependencies sendScanStruct;
        sendScanStruct.customMsgHandler = customSendScanToOverseer;
        sendScanStruct.remotePort = portO;
        sendScanStruct.remoteAddr = addressO;
        sendScanStruct.moduleName = moduleName;
        sendScanStruct.arguments = argv;

        connectToRemoteSocketAndSendMessage((void*)&sendScanStruct);
        pthread_cond_signal(&response_cond);
        }else{
        memcpy(scanned, p->scanned, sizeof(scanned));
        struct CustomSendMsgHandlerAndDependencies sendScanStruct;
        sendScanStruct.customMsgHandler = customSendScanToOverseer;
        sendScanStruct.remotePort = portO;
        sendScanStruct.remoteAddr = addressO;
        sendScanStruct.moduleName = moduleName;
        sendScanStruct.arguments = argv;

        connectToRemoteSocketAndSendMessage((void*)&sendScanStruct);
        pthread_cond_signal(&response_cond);
        }
    }


}

//{procname} {id} {wait time (in microseconds)} {shared memory path} {shared memory offset} {overseer address:port}
int main(int argc, char* argv[]){
    cardreader(argc, argv);
    return 1;
}

