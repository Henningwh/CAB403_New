#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "ipLibTCP.h"

char moduleName[20];
pthread_mutex_t emergMutex = PTHREAD_MUTEX_INITIALIZER;;
int emergencyMode = 0;
char status; // 'O' for open, 'C' for closed, 'o' for opening, 'c' for closing
pthread_mutex_t mutex;
pthread_cond_t cond_start;
pthread_cond_t cond_end;

void customHandleRecieveMessagesInDoor(struct CustomMsgHandlerArgs* sockAndargs){
    char* msg1 = recieveAndPrintMsg(sockAndargs->socket, moduleName);
    if((strcmp(msg1, "OPEN") == 0 && status == 'O') || (strcmp(msg1, "CLOSE") == 0 && status == 'C')){
        sendAndPrintFromModule(moduleName, "ALREADY#");
        close(sockAndargs->socket);
    }else if(strcmp(msg1, "OPEN") == 0 && status == 'C'){
        sendAndPrintFromModule(moduleName, "OPENING#", sockAndargs->socket);
        pthread_mutex_lock(&mutex);
        status = 'o';
        pthread_cond_signal(&cond_start);
        pthread_cond_wait(&cond_end, &mutex);
        //ToDo: Not in desc, but makes sense:
        status = 'O'
        pthread_mutex_unlock(&mutex);
        sendAndPrintFromModule(moduleName, "OPENED#", sockAndargs->socket);
        close(sockAndargs.socket);
    }else if(strcmp(msg1, "CLOSE") == 0 && status == 'O'){
        sendAndPrintFromModule(moduleName, "CLOSING#", sockAndargs->socket);
        pthread_mutex_lock(&mutex);
        status = 'c';
        pthread_cond_signal(&cond_start);
        pthread_cond_wait(&cond_end, &mutex);
        //ToDo: Not in desc, but makes sense:
        status = 'C'
        pthread_mutex_unlock(&mutex);
        sendAndPrintFromModule(moduleName, "CLOSED#", sockAndargs->socket);
        close(sockAndargs.socket);
    }else if(strcmp(msg1, "OPEN_EMERG") == 0){
        //Locking mutex very long, but it has priority since its an emergency.
        pthread_mutex_lock(mutex);
        if(status == 'C');
        pthread_mutex_unlock(mutex);


        pthread_mutex_lock(emergMutex);
        emergencyMode = 1;
        pthread_mutex_unlock(emergMutex);

    }
}
void door(int argc, char *argv[]) {
    moduleName = strcat("Door: ", argV[1]);

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_start, NULL);
    pthread_cond_init(&cond_end, NULL);

    char **resultArray = (char **)malloc(10 * sizeof(char *));
    char *input = argV[2];
    int maxSeqments = 10;
    splitString(input, ":", resultArray, maxSeqments);
    char* address = strdup(resultArray[0]);
    int port = atoi(resultArray[1]);
    free(resultArray);

        //Ip TCP stuff///////////////////////
    int listenTCPSocketFD = openAndBindNewTCPport(port, moduleName);
    struct CustomRecieveMsgHandlerAndDependencies doorMsgHandlerStruct;
    doorMsgHandlerStruct.customMsgHandler = customHandleRecieveMessagesInDoor;
    doorMsgHandlerStruct.listenSocketFD = listenTCPSocketFD;
    doorMsgHandlerStruct.moduleName = moduleName;
    doorMsgHandlerStruct.arguments = argV;

    //Continously listening for connections and queueing up to 100
    int connections = listen(listenTCPSocketFD,100);
    printf("%s: started listening for incomming connections...\n", moduleName);
    //accepting connections and messages in separate thread
    pthread_t id;
    pthread_create(&id,NULL,continouslyAcceptConnections,(void*)&doorMsgHandlerStruct);

}

//{progname} {id} {address:port} {FAIL_SAFE | FAIL_SECURE} {shared memory path} {shared memory offset} {overseer address:port}
int main(int argc, char* argv[]){
    door(argc, argv);
}