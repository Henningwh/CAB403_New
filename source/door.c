#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s {id} {address:port} {FAIL_SAFE | FAIL_SECURE} {shared memory path} {shared memory offset} {overseer address:port}\n", argv[0]);
        return 1; // Exit with an error code
    }

    // Parse command-line arguments and initialize door controller
    char *id = argv[1];
    char *address = argv[2];
    char *mode = argv[3];
    char *shm_path = argv[4];
    int shm_offset = atoi(argv[5]);
    char *overseer_addr = argv[6];
    
    return 0; // Return 0 to indicate successful execution
}
// Shared memory structure
typedef struct {
    char status; // 'O' for open, 'C' for closed, 'o' for opening, 'c' for closing
    pthread_mutex_t mutex;
    pthread_cond_t cond_start;
    pthread_cond_t cond_end;
} DoorMemory;

DoorMemory doorMemory;

pthread_mutex_t overseer_mutex = PTHREAD_MUTEX_INITIALIZER;

void send_message_to_overseer(const char* message, const char* overseer_addr) {
    pthread_mutex_lock(&overseer_mutex);
    // TODO: Code to send message to overseer using overseer_addr
    printf("Sending message to overseer: %s\n", message);
    // Simulating delay
    sleep(1);
    pthread_mutex_unlock(&overseer_mutex);
}

void init_door(char* id, char* address, char* mode, char* shm_path, int shm_offset, char* overseer_addr) {
    // Initialize shared memory, TCP connections, and other necessary things here...
    doorMemory.status = 'C'; // Default status is closed
    pthread_mutex_init(&doorMemory.mutex, NULL);
    pthread_cond_init(&doorMemory.cond_start, NULL);
    pthread_cond_init(&doorMemory.cond_end, NULL);

    // Send initialization message to overseer
    char message[256];
    sprintf(message, "DOOR %s %s %s#", id, address, mode);
    send_message_to_overseer(message, overseer_addr);
}

void *handle_connection(void *arg) {
    int *new_socket = (int *)arg; // Cast the argument correctly

    // Pseudo code for handling TCP connection based on the given logic
    char message[256];
    // Read the message from the connection...

    pthread_mutex_lock(&doorMemory.mutex);
    char doorStatus = doorMemory.status;
    pthread_mutex_unlock(&doorMemory.mutex);

    if (strcmp(message, "OPEN#") == 0) {
        if (doorStatus == 'O') {
            // Respond with ALREADY#...
        } else if (doorStatus == 'C') {
            // Respond with OPENING#...
            pthread_mutex_lock(&doorMemory.mutex);
            doorMemory.status = 'o';
            pthread_cond_signal(&doorMemory.cond_start);
            pthread_cond_wait(&doorMemory.cond_end, &doorMemory.mutex);
            doorMemory.status = 'O';
            pthread_mutex_unlock(&doorMemory.mutex);
            // Respond with OPENED#...
        }
    } else if (strcmp(message, "CLOSE#") == 0) {
        if (doorStatus == 'C') {
            // Respond with ALREADY#...
        } else if (doorStatus == 'O') {
            // Respond with CLOSING#...
            pthread_mutex_lock(&doorMemory.mutex);
            doorMemory.status = 'c';
            pthread_cond_signal(&doorMemory.cond_start);
            pthread_cond_wait(&doorMemory.cond_end, &doorMemory.mutex);
            doorMemory.status = 'C';
            pthread_mutex_unlock(&doorMemory.mutex);
            // Respond with CLOSED#...
        }
    } else if (strcmp(message, "OPEN_EMERG#") == 0) {
        // Respond with OPENING_EMERG#...
        // TODO: Implement emergency opening logic here...
        // Respond with OPENED_EMERG#...
    } else if (strcmp(message, "CLOSE_SECURE#") == 0) {
        // Respond with CLOSING_SECURE#...
        // TODO: Implement secure closing logic here...
        // Respond with CLOSED_SECURE#...
    }

    close(*new_socket); // Close the socket
    return NULL;
}

void door(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s {id} {address:port} {FAIL_SAFE | FAIL_SECURE} {shared memory path} {shared memory offset} {overseer address:port}\n", argv[0]);
        return;
    }
    init_door(argv[0], argv[1], argv[2], argv[3], atoi(argv[4]), argv[5]);

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[1]));
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Loop to handle incoming connections
    while (1) {
        // Accept incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Create a new thread to handle the connection
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_connection, (void *)&new_socket) < 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
}