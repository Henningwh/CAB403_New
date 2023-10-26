#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>
#include "ipLib.h"
#include "simpleTestModule.h"

char* moduleN = "Test module";
void customHandleRecieveMessagesInTestModule(int remoteSocketFD){
    char buffer[1024]; 
   while(true){
    ssize_t  msgCount= recv(remoteSocketFD,buffer,1024,0);

        if(msgCount>0)
        {
            buffer[msgCount] = 0;
            printf("Test Module recieved: %s \n",buffer);
        }
   }
}


void customHandleSendMessages(int localSocketFD){
    printf("TestModule: inside custom send messages\n");
    char* greeting = "Hi from test module#";
    sendAndPrintFromModule(moduleN, greeting, localSocketFD);
    char* msg1 = recieveAndPrintMsg(localSocketFD,moduleN);
    if(strcmp(msg1, "Hi back from overseer") == 0){
        char* sendstuff = "please send stuff#";
        sendAndPrintFromModule(moduleN, sendstuff, localSocketFD);
    }else{
        printf("test module: got the wrong message: %s\n", msg1);
    }
    char* msg2 = recieveAndPrintMsg(localSocketFD,moduleN);
    if(strcmp(msg2, "here is stuff")==0){
        printf("Test module got STUFF. closing socket.\n");
        close(localSocketFD);
    }else{
        printf("test module: got the wrong message. Should be: here is stuff: %s\n", msg2);
    }


}


void runTestModule(){
    //int port = 6000;
    //int listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    //struct sockaddr_in* listenAddr = createAddress(port, "");
    //lets the Test module accept all connections since no ip address was specified in listenAddr
    //int bindRes = bind(listenSocketFD, listenAddr, sizeof(*listenAddr));
    //if(bindRes == 0){
    //    printf("Test module: Listen socket bound to port %d\n", port);
    //}else{
    //    printf("Test module: Bind to port %d FAILED\n", port);
    //}

    struct CustomSendMsgHandlerAndDependencies overseerSendMsgStruct;
    overseerSendMsgStruct.customMsgHandler = customHandleSendMessages;
    overseerSendMsgStruct.remotePort = 3000;
    overseerSendMsgStruct.remoteAddr = "127.0.0.1";
    overseerSendMsgStruct.moduleName = "Test module";

    connectToRemoteSocketAndSendMessage((void*)&overseerSendMsgStruct);
}

