
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>
#include "ipLibTCP.h"
#include "ipLibUDP.h"
#include "overseer.h"
#include "sleepFunction.h"
#include "dg_structs.h"

#define MAX_LINE_LENGTH 1024

pthread_mutex_t secureDoorMutex = PTHREAD_MUTEX_INITIALIZER;
struct DoorInfo secureDoors[10];
int secureDoorsCount = 0;

pthread_mutex_t safeDoorMutex = PTHREAD_MUTEX_INITIALIZER;
struct DoorInfo safeDoors[10];
int safeDoorsCount = 0;

void customHandleRecieveFromTestModule(char *msg, int remoteSocketFD);
char **sentenceToWordArray(char *sentence);

struct DoorInfo *findDoorById(struct DoorInfo secureDoors[], struct DoorInfo safeDoors[], int id)
{
    for (int i = 0; i < 10; i++)
    {
        if (secureDoors[i].id == id)
        {
            return &secureDoors[i];
        }
        if (safeDoors[i].id == id)
        {
            return &safeDoors[i];
        }
    }
    return NULL; // No matching ID found
}

// Function to check if a card reader is allowed to access a specific device
int checkAccess(char accessCode[], int cardId, char deviceId[], char authFile[], char connFile[])
{
    char line[MAX_LINE_LENGTH];
    char aLine[MAX_LINE_LENGTH];
    FILE *authFilePtr = fopen(authFile, "r");
    FILE *connFilePtr = fopen(connFile, "r");

    if (authFilePtr == NULL || connFilePtr == NULL)
    {
        perror("Error opening files");
        exit(1);
    }

    while (fgets(line, MAX_LINE_LENGTH, connFilePtr) != NULL)
    {
        char connDevice[MAX_LINE_LENGTH];
        int connCardId;
        int connDeviceId;
        if (sscanf(line, "%s %d %d", connDevice, &connCardId, &connDeviceId) == 3 && connCardId == cardId && strcmp(connDevice, deviceId) == 0)
        {
            memset(line, 0, MAX_LINE_LENGTH);
            while (fgets(aLine, MAX_LINE_LENGTH, authFilePtr) != NULL)
            {
                char authCode[MAX_LINE_LENGTH];
                if (sscanf(aLine, "%s", authCode) == 1 && strcmp(authCode, accessCode) == 0)
                {
                    char **wordArr = sentenceToWordArray(aLine);
                    for (int i = 1; i < 3; i++)
                    {
                        char *id = wordArr[i] + 5;
                        id[strcspn(id, "\n")] = 0;
                        int b = atoi(id);

                        if (b == connDeviceId)
                        {

                            fclose(authFilePtr);
                            fclose(connFilePtr);
                            return b;
                        }
                    }
                }
            }
        }
    }

    fclose(authFilePtr);
    fclose(connFilePtr);
    return 0; // Access not allowed
}

char *getFirstWord(const char *input)
{
    static char result[100];
    int i = 0;

    // Iterate through the input string
    while (input[i] != ' ' && input[i] != '\0')
    {
        result[i] = input[i];
        i++;
    }

    return result;
}

/**
 * Handles cli inputs.
 */
void handleCommand(char *command)
{
}

char *moduleName = "Overseer";

void customHandleRecieveFromTestModule(char *msg, int remoteSocketFD)
{
    if (strcmp(msg, "Hi from test module") == 0)
    {
        sendAndPrintFromModule(moduleName, "Hi back from overseer#", remoteSocketFD);
        char *gotStuff = recieveAndPrintMsg(remoteSocketFD, moduleName);
        if (strcmp(gotStuff, "please send stuff") == 0)
        {
            sendAndPrintFromModule(moduleName, "here is stuff#", remoteSocketFD);
        }
        else
        {
            printf("Overseer: Supposed to get 'please send stuff#'. Got: %s\n", gotStuff);
        }
    }
    else
    {
        printf("Overseer: Supposed to get 'Hi from test module#'. Got: %s\n", msg);
    }
}

char **sentenceToWordArray(char *sentence)
{
    int offset = 0;
    int wordIndex = 0;
    char **wordArray = (char **)malloc(10 * sizeof(char *)); // Dynamically allocate memory
    size_t charCount = strlen(sentence);
    char word[charCount];

    for (int i = 0; i < charCount; i++)
    {
        if (sentence[i] == ' ')
        {
            word[i - offset] = '\0';
            wordArray[wordIndex] = strdup(word);
            wordIndex++;
            offset = i + 1;
            continue;
        }
        word[i - offset] = sentence[i];
    }

    // Handle the last word
    word[charCount - offset] = '\0';
    wordArray[wordIndex] = strdup(word);

    return wordArray;
}

void customSendAllowedToDoor(struct CustomMsgHandlerArgs *sockAndargs)
{
    sendAndPrintFromModule(moduleName, "OPEN#", sockAndargs->socket);
    char *msg1 = recieveAndPrintMsg(sockAndargs->socket, moduleName);
    if (strcmp(msg1, "OPENING") == 0)
    {
        char *msg2 = recieveAndPrintMsg(sockAndargs->socket, moduleName);
        if (strcmp(msg2, "OPENED") == 0)
        {
            int microsecondsOpen = atoi(sockAndargs->arguments[3]);
            sleepMicroseconds(microsecondsOpen);
            sendAndPrintFromModule(moduleName, "CLOSE#", sockAndargs->socket);
        }
        else
        {
            printf("Got the wrong message, should have gotten OPENED, got: %s", msg2);
            exit(1);
        }
    }
    else
    {
        printf("Got the wrong message, should have gotten OPENING, got: %s", msg1);
        exit(1);
    }
}

// CARDREADER 101 HELLO#
// CARDREADER 101 SCANNED 0b9adf9c81fb959#
void customHandleCardreader(char *msg, char *authFile, char *connFile, struct CustomMsgHandlerArgs *sockAndargs)
{

    char **wordArr = sentenceToWordArray(msg);
    if (strcmp(wordArr[2], "HELLO") == 0)
    {
        struct CardReaderInfo cardReaderStruct;
        cardReaderStruct.id = atoi(wordArr[1]);
    }
    else if (strcmp(wordArr[2], "SCANNED") == 0)
    {
        printf("GOT INSIDE SCANNED: %s, %d, %s, %s\n", wordArr[3], atoi(wordArr[1]), authFile, connFile);
        int doorId = checkAccess(wordArr[3], atoi(wordArr[1]), "DOOR", authFile, connFile);
        if (doorId != 0)
        {
            printf("%s: Cardscanner %d ALLOWED: %d\n", moduleName, atoi(wordArr[1]), doorId);
            sendAndPrintFromModule(moduleName, "ALLOWED#", sockAndargs->socket);
            close(sockAndargs->socket);
            struct DoorInfo *doorStruct = findDoorById(secureDoors, safeDoors, doorId);
            struct CustomSendMsgHandlerAndDependencies *sendDoorStruct;
            sendDoorStruct->remoteAddr = doorStruct->ip;
            sendDoorStruct->remotePort = doorStruct->port;
            sendDoorStruct->arguments = sockAndargs->arguments;
            sendDoorStruct->customMsgHandler = customSendAllowedToDoor;
            connectToRemoteSocketAndSendMessage(sendDoorStruct);
        }
        else
        {
            printf("%s: Cardscanner %d DENIED: %d\n", moduleName, atoi(wordArr[1]), doorId);
        }
    }
    else
    {
        printf("%s: Did not recognise command: %s for: %s", moduleName, wordArr[2], wordArr[0]);
        exit(1);
    }
}
// DOOR {id} {address:port} {FAIL_SAFE | FAIL_SECURE}#
void customHandleDoor(char *msg)
{
    char **wordArr = sentenceToWordArray(msg);

    if (strcmp(wordArr[3], "FAIL_SAFE") == 0)
    {
        char **ipArr = (char **)malloc(10 * sizeof(char *));
        splitString(wordArr[2], ":", ipArr, 3);
        struct DoorInfo doorStruct;
        doorStruct.id = atoi(wordArr[1]);
        doorStruct.ip = ipArr[0];
        doorStruct.port = atoi(ipArr[1]);
        doorStruct.configuration = wordArr[3];

        pthread_mutex_lock(&safeDoorMutex);
        safeDoors[safeDoorsCount] = doorStruct;
        safeDoorsCount++;
        pthread_mutex_unlock(&safeDoorMutex);
        printf("%s: registered Door: %d as a safe door\n", moduleName, doorStruct.id);
        // findDoorAndReturnStruct()
    }
    else if (strcmp(wordArr[3], "FAIL_SECURE") == 0)
    {
        char **ipArr = (char **)malloc(10 * sizeof(char *));
        splitString(wordArr[2], ":", ipArr, 3);
        struct DoorInfo doorStruct;
        doorStruct.id = atoi(wordArr[1]);
        doorStruct.ip = ipArr[0];
        doorStruct.port = atoi(ipArr[1]);
        doorStruct.configuration = wordArr[3];

        pthread_mutex_lock(&secureDoorMutex);
        secureDoors[secureDoorsCount] = doorStruct;
        secureDoorsCount++;
        pthread_mutex_unlock(&secureDoorMutex);
        printf("%s: registered Door: %d as a secure door\n", moduleName, doorStruct.id);
    }
    else
    {
        printf("%s: Did not recognise command: %s for: %s", moduleName, wordArr[3], wordArr[0]);
        exit(1);
    }
}
// FIREALARM {address:port} HELLO#
void customHandleFirealarm(char *msg)
{
    char **wordArr = sentenceToWordArray(msg);
    char **ipArr = (char **)malloc(10 * sizeof(char *));
    splitString(wordArr[2], ":", ipArr, 3);
    struct FireAlarmInfo fireAlarmStruct;
    fireAlarmStruct.id = atoi(wordArr[1]);
    fireAlarmStruct.ip = ipArr[0];
    fireAlarmStruct.port = atoi(ipArr[1]);
}
// ELEVATOR {id} {address:port} HELLO#
void customHandleElevator(char *msg)
{
}
// DESTSELECT {id} HELLO#
void customHandleDestselect(char *msg)
{
}

void customHandleRecieveMessagesInOverseer(struct CustomMsgHandlerArgs *sockAndargs)
{
    char *msg = recieveAndPrintMsg(sockAndargs->socket, moduleName);
    char *firstWord = getFirstWord(msg);
    printf("First word: %s\n", firstWord);

    if (strcmp(firstWord, "CARDREADER") == 0)
    {
        customHandleCardreader(msg, sockAndargs->arguments[5], sockAndargs->arguments[6], sockAndargs);
    }
    else if (strcmp(firstWord, "DOOR") == 0)
    {
        customHandleDoor(msg);
    }
    else if (strcmp(firstWord, "FIREALARM") == 0)
    {
        customHandleFirealarm(msg);
    }
    else if (strcmp(firstWord, "ELEVATOR") == 0)
    {
        customHandleElevator(msg);
    }
    else if (strcmp(firstWord, "DESTSELECT") == 0)
    {
        customHandleDestselect(msg);
    }
    else
    {
        printf("Unknown device type: %s\n", firstWord);
        exit(1);
    }

    // customHandleRecieveFromTestModule(msg, remoteSocketFD);
}

// UDP Handlers//////////////////////////
void customParseAndHandleUdp(temp_update_dg *msg)
{
    printf("Overseer: recieved UDP message from %d\n", msg->id);
}

void overseer(char *argV[])
{
    printf("Inside overseer\n");
    for (int i = 0; i < 8; i++)
    {
        printf("argV[%d]: %s\n", i, argV[i]);
    }

    char **resultArray = (char **)malloc(10 * sizeof(char *));
    char *input = argV[2];
    int maxSeqments = 10;
    splitString(input, ":", resultArray, maxSeqments);
    char *address = strdup(resultArray[0]);
    int port = atoi(resultArray[1]);
    free(resultArray);

    printf("%s: Address: %s, Port: %d\n", moduleName, address, port);

    // Ip TCP stuff///////////////////////
    int listenTCPSocketFD = openAndBindNewTCPport(port, moduleName);
    struct CustomRecieveMsgHandlerAndDependencies overseerMsgHandlerStruct;
    overseerMsgHandlerStruct.customMsgHandler = customHandleRecieveMessagesInOverseer;
    overseerMsgHandlerStruct.listenSocketFD = listenTCPSocketFD;
    overseerMsgHandlerStruct.moduleName = moduleName;
    overseerMsgHandlerStruct.arguments = argV;

    // Continously listening for connections and queueing up to 100
    int connections = listen(listenTCPSocketFD, 100);
    printf("%s: started listening for incomming connections...\n", moduleName);
    // accepting connections and messages in separate thread
    pthread_t id;
    pthread_create(&id, NULL, continouslyAcceptConnections, (void *)&overseerMsgHandlerStruct);

    // IP UDP Stuff//////////////////////////
    int listenUDPSocketFD = openAndBindNewUdpPort(overseerUdpPort, moduleName);
    pthread_t id2;
    struct RecieveUDPMsgStruct overseerUdpParseAndHandle;
    overseerUdpParseAndHandle.moduleName = moduleName;
    overseerUdpParseAndHandle.listenSocketFD = listenUDPSocketFD;
    overseerUdpParseAndHandle.customParseAndHandleMessage = customParseAndHandleUdp;
    pthread_create(&id2, NULL, continouslyRecieveUDPMsgAndPrintTemp, (void *)&overseerUdpParseAndHandle);

    char *command = NULL;
    size_t commandLen = 0;
    printf("%s: Ready for cli commands:\n", moduleName);

    char buffer[1024];

    while (true)
    {
        ssize_t charCount = getline(&command, &commandLen, stdin);
        command[charCount - 1] = 0;
        printf("command: %s \n", command);
        if (strcmp(command, "exit") == 0)
        {
            break;
        }
        handleCommand(command);
    }
}

int main(int argc, char *argv[])
{
    overseer(argv);
}