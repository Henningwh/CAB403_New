#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Structure for shared memory
struct SharedMemory {
    char scanned[16];
    uint8_t floor_select;
    pthread_mutex_t mutex;
    pthread_cond_t scanned_cond;
    char response;
    pthread_cond_t response_cond;
};

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
    struct SharedMemory *shm;
    // TODO: Attach to shared memory segment using shm_open, mmap, and shm_offset

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
        int socket_fd;
        // TODO: Create a socket and connect to overseer_addr_port

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
        if (bytes_received > 0 && response == 'A') {
            shm->response = 'Y';
        } else {
            shm->response = 'N';
        }

        // Signal response_cond
        pthread_cond_signal(&shm->response_cond);
    }

    return 0;
}
