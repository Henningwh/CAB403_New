#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "ipLibTCP.h"

char* moduleName = "Door";
pthread_mutex_t emergMutex = PTHREAD_MUTEX_INITIALIZER;
int emergencyMode = 0;

pthread_mutex_t secMutex = PTHREAD_MUTEX_INITIALIZER;;
int securityMode = 0;

char status; // 'O' for open, 'C' for closed, 'o' for opening, 'c' for closing
pthread_mutex_t mutex;
pthread_cond_t cond_start;
pthread_cond_t cond_end;

void customHandleRecieveMessagesInDoor(struct CustomMsgHandlerArgs* sockAndargs){
    //Secure = 0, Safe = 1

    int doorMode = 0;
    if(strcmp(sockAndargs->arguments[3], "FAIL_SAFE")){doorMode = 1;};

    char* msg1 = recieveAndPrintMsg(sockAndargs->socket, moduleName);

    if(strcmp(msg1, "OPEN") == 0){

        pthread_mutex_lock(&emergMutex);
        int safeEmergMode = emergencyMode;
        pthread_mutex_unlock(&emergMutex);

        pthread_mutex_lock(&secMutex);
        int safeSecMode = securityMode;
        pthread_mutex_unlock(&secMutex);

        if(safeEmergMode == 1 && doorMode){
            sendAndPrintFromModule(moduleName, "EMERGENCY_MODE#", sockAndargs->socket);
            close(sockAndargs->socket);
        }
        else if(safeSecMode == 1 && !doorMode){
            sendAndPrintFromModule(moduleName, "SECURE_MODE#", sockAndargs->socket);
            close(sockAndargs->socket);
        }
        else if(status == 'O'){
            sendAndPrintFromModule(moduleName, "ALREADY#", sockAndargs->socket);
            close(sockAndargs->socket);
        }
        else if(status == 'C'){
            sendAndPrintFromModule(moduleName, "OPENING#", sockAndargs->socket);
            pthread_mutex_lock(&mutex);
            status = 'o';
            pthread_cond_signal(&cond_start);
            pthread_cond_wait(&cond_end, &mutex);
            //ToDo: Not in desc, but makes sense:
            status = 'O';
            pthread_mutex_unlock(&mutex);
            sendAndPrintFromModule(moduleName, "OPENED#", sockAndargs->socket);
            close(sockAndargs->socket);
        }
        
    }else if(strcmp(msg1, "CLOSE") == 0){

        pthread_mutex_lock(&emergMutex);
        int safeEmergMode = emergencyMode;
        pthread_mutex_unlock(&emergMutex);

        pthread_mutex_lock(&secMutex);
        int safeSecMode = securityMode;
        pthread_mutex_unlock(&secMutex);

        if(safeEmergMode == 1 && doorMode){
                sendAndPrintFromModule(moduleName, "EMERGENCY_MODE#", sockAndargs->socket);
                close(sockAndargs->socket);
            }
        else if(safeSecMode == 1 && !doorMode){
                sendAndPrintFromModule(moduleName, "SECURE_MODE#", sockAndargs->socket);
                close(sockAndargs->socket);
            }
        else if(status == 'C'){
            sendAndPrintFromModule(moduleName, "ALREADY#", sockAndargs->socket);
            close(sockAndargs->socket);
        }
        else if(status == 'O'){
            sendAndPrintFromModule(moduleName, "CLOSING#", sockAndargs->socket);
            pthread_mutex_lock(&mutex);
            status = 'c';
            pthread_cond_signal(&cond_start);
            pthread_cond_wait(&cond_end, &mutex);
            //ToDo: Not in desc, but makes sense:
            status = 'C';
            pthread_mutex_unlock(&mutex);
            sendAndPrintFromModule(moduleName, "CLOSED#", sockAndargs->socket);
            close(sockAndargs->socket);
        }
    }else if(strcmp(msg1, "OPEN_EMERG") == 0 && doorMode){
        //Locking mutex very long, but it has priority since its an emergency.
        pthread_mutex_lock(&emergMutex);
        emergencyMode = 1;
        pthread_mutex_unlock(&emergMutex);
        pthread_mutex_lock(&mutex);
        if(status == 'C'){
            sendAndPrintFromModule(moduleName, "OPENING#", sockAndargs->socket);
            status = 'o';
            pthread_cond_signal(&cond_start);
            pthread_cond_wait(&cond_end, &mutex);
            //ToDo: Not in desc, but makes sense:
            status = 'O';
            sendAndPrintFromModule(moduleName, "OPENED#", sockAndargs->socket);
            printf("%s: Emergency mode, opened door, closing connection.\n",moduleName);
            close(sockAndargs->socket);
        }else{
            printf("%s: Emergency mode, allready open, closing connection.\n",moduleName);
            close(sockAndargs->socket);
        }
        pthread_mutex_unlock(&mutex);
    }else if(strcmp(msg1, "CLOSE_SECURE") == 0 && !doorMode){
        //Locking mutex very long, but it has priority since its an emergency.
        pthread_mutex_lock(&secMutex);
        securityMode = 1;
        pthread_mutex_unlock(&secMutex);
        pthread_mutex_lock(&mutex);
        if(status == 'O'){
            sendAndPrintFromModule(moduleName, "CLOSING#", sockAndargs->socket);
            status = 'c';
            pthread_cond_signal(&cond_start);
            pthread_cond_wait(&cond_end, &mutex);
            //ToDo: Not in desc, but makes sense:
            status = 'C';
            sendAndPrintFromModule(moduleName, "CLOSED#", sockAndargs->socket);
            printf("%s: Security mode, closed door, closing connection.\n",moduleName);
            close(sockAndargs->socket);
        }else{
            printf("%s: Security mode, allready closed, closing connection.\n",moduleName);
            close(sockAndargs->socket);
        }
        pthread_mutex_unlock(&mutex);
    }
}
void door(int argc, char *argv[]) {
    char* idFromArg = argv[1];

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_start, NULL);
    pthread_cond_init(&cond_end, NULL);

    printf("INitialized mutexes\n");

    char **resultArray = (char **)malloc(10 * sizeof(char *));
    char *input = argv[2];
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
    doorMsgHandlerStruct.arguments = argv;

    //Continously listening for connections and queueing up to 100
    int connections = listen(listenTCPSocketFD,100);
    printf("%s: started listening for incomming connections...\n", moduleName);
    //accepting connections and messages in separate thread
    pthread_t id;
    pthread_create(&id,NULL,continouslyAcceptConnections,(void*)&doorMsgHandlerStruct);

    pthread_join(&id, NULL);

}

//{progname} {id} {address:port} {FAIL_SAFE | FAIL_SECURE} {shared memory path} {shared memory offset} {overseer address:port}
int main(int argc, char* argv[]){
    door(argc, argv);
}