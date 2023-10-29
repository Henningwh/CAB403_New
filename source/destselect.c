#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "data_structs.h"
#include "dg_structs.h"
#include "ipLibTCP.h"

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s {id} {wait time (in microseconds)} {shared memory path} {shared memory offset} {overseer address:port}\n", argv[0]);
        exit(1);
    }

    int id = atoi(argv[1]);
    int wait_time = atoi(argv[2]);
    const char *shm_path = argv[3];
    int shm_offset = atoi(argv[4]);
    const char *overseer_addr_port = argv[5];

    // Attach to the shared memory segment
    shm_destselect *shm;
    int shm_fd = shm_open(shm_path, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    shm = mmap(0, sizeof(shm_destselect), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, shm_offset);
    if (shm == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    //{prossid} {id} {wait time (in microseconds)} {shared memory path} {shared memory offset} {overseer address:port}
    char* addres = argv[5];
    struct CustomSendMsgHandlerAndDependencies overseerSendMsgStruct;
    overseerSendMsgStruct.customMsgHandler = customHandleSendMessages;
    overseerSendMsgStruct.remotePort = 3000;
    overseerSendMsgStruct.remoteAddr = "127.0.0.1";
    overseerSendMsgStruct.moduleName = "Test module";

    connectToRemoteSocketAndSendMessage((void*)&overseerSendMsgStruct);



    while (1) {
        // Lock the mutex
        pthread_mutex_lock(&shm->mutex);

        // Check if a card has been scanned
        if (strcmp(shm->scanned, "") == 0) {
            // Wait for card swipe
            pthread_cond_wait(&shm->scanned_cond, &shm->mutex);
        }

        // Get card data and selected floor
        char card_data[17];
        strcpy(card_data, shm->scanned);
        int selected_floor = shm->floor_select;

        // Unlock the mutex
        pthread_mutex_unlock(&shm->mutex);

        // Open a TCP connection to the overseer
        int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd == -1) {
            perror("socket");
            continue;
        }

        struct sockaddr_in overseer_addr;
        
        char overseer_ip[100];
        int overseer_port;
        if (sscanf(overseer_addr_port, "%99[^:]:%d", overseer_ip, &overseer_port) != 2) {
            fprintf(stderr, "Invalid overseer address:port format\n");
            exit(EXIT_FAILURE);
        }

        overseer_addr.sin_family = AF_INET;
        overseer_addr.sin_port = htons(overseer_port);
        if (inet_pton(AF_INET, overseer_ip, &overseer_addr.sin_addr) <= 0) {
            perror("inet_pton");
            exit(EXIT_FAILURE);
        }

        if (connect(socket_fd, (struct sockaddr*)&overseer_addr, sizeof(overseer_addr)) == -1) {
            perror("connect");
            close(socket_fd);
            continue;
        }

        // Send data to overseer
        char message[1024];
        sprintf(message, "DESTSELECT %d SCANNED %s %d#", id, card_data, selected_floor);
        send(socket_fd, message, strlen(message), 0);

        // Wait for a response with a timeout
        char response;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = wait_time;
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        int bytes_received = recv(socket_fd, &response, 1, 0);

        // Close the socket
        close(socket_fd);

        // Process the response
        pthread_mutex_lock(&shm->mutex);
        if (bytes_received > 0 && response == 'A') {
            shm->response = 'Y';
        } else {
            shm->response = 'N';
        }
        pthread_cond_signal(&shm->response_cond);
        pthread_mutex_unlock(&shm->mutex);
    }

    return 0;
}
