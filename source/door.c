#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "helper_functions.h"
#include "shm_structs.h"
#include "dg_structs.h"

shm_door doorMemory;
pthread_mutex_t overseer_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_door(char* id, char* address, char* mode, char* shm_path, int shm_offset, char* overseer_addr) {
    pthread_mutex_init(&doorMemory.mutex, NULL);
    pthread_cond_init(&doorMemory.cond_start, NULL);
    pthread_cond_init(&doorMemory.cond_end, NULL);
    doorMemory.status = 'C'; // Default status is closed
    char message[256];
    sprintf(message, "DOOR %s %s %s#", id, address, mode);
    send_message_to_overseer(message, overseer_addr);
}

void *handle_connection(void *arg) {
    int new_socket = *((int *)arg);
    free(arg);
    // Handle TCP connection here...
    close(new_socket);
    return NULL;
}

void cleanup() {
    pthread_mutex_destroy(&doorMemory.mutex);
    pthread_cond_destroy(&doorMemory.cond_start);
    pthread_cond_destroy(&doorMemory.cond_end);
    pthread_mutex_destroy(&overseer_mutex);
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr, "Usage: %s {id} {address} {port} {FAIL_SAFE | FAIL_SECURE} {shared memory path} {shared memory offset} {overseer address:port}\n", argv[0]);
        return 1;
    }
    char *id = argv[1];
    char *address = argv[2];
    int port = atoi(argv[3]);
    char *mode = argv[4];
    char *shm_path = argv[5];
    int shm_offset = atoi(argv[6]);
    char *overseer_addr = argv[7];

    init_door(id, address, mode, shm_path, shm_offset, overseer_addr);

    int server_fd, new_socket;
    struct sockaddr_in server_addr;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        return 1;
    }

    while (1) {
        if ((new_socket = accept(server_fd, NULL, NULL)) < 0) {
            perror("accept");
            return 1;
        }

        int *pclient = malloc(sizeof(int));
        *pclient = new_socket;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_connection, pclient) < 0) {
            perror("pthread_create");
            free(pclient);
            return 1;
        }
    }

    cleanup();
    return 0;
}
