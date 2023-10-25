#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

// Shared memory structure
struct shared_memory {
    char alarm; // '-' if inactive, 'A' if active
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

// Datagram format
struct datagram {
    char header[4]; // {'D', 'O', 'O', 'R'}
    struct in_addr door_addr;
    in_port_t door_port;
};

// Door registration datagram
struct door_reg_datagram {
    char header[4]; // {'D', 'R', 'E', 'G'}
    struct in_addr door_addr;
    in_port_t door_port;
};

// Temperature update datagram
struct temp_datagram {
    char header[4]; // {'T', 'E', 'M', 'P'}
    float temperature;
    long timestamp;
};

// Fire emergency datagram
struct fire_datagram {
    char header[4]; // {'F', 'I', 'R', 'E'}
};

// Open emergency door message
const char* OPEN_EMERG_MSG = "OPEN_EMERG#";

// Function to send a door confirmation datagram to the overseer
void send_door_confirmation(int overseer_sock, struct sockaddr_in overseer_addr, struct in_addr door_addr, in_port_t door_port) {
    struct door_reg_datagram door_reg = {{'D', 'R', 'E', 'G'}, door_addr, door_port};
    sendto(overseer_sock, &door_reg, sizeof(door_reg), 0, (struct sockaddr*)&overseer_addr, sizeof(overseer_addr));
}

// Function to open an emergency door
void open_emergency_door(struct in_addr door_addr, in_port_t door_port) {
    struct sockaddr_in door_sock_addr;
    int door_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(door_sock == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    memset(&door_sock_addr, 0, sizeof(door_sock_addr));
    door_sock_addr.sin_family = AF_INET;
    door_sock_addr.sin_port = htons(door_port);
    door_sock_addr.sin_addr = door_addr;

    if(connect(door_sock, (struct sockaddr*)&door_sock_addr, sizeof(door_sock_addr)) == -1) {
        perror("Failed to connect to door");
        close(door_sock);
        return;
    }

    if(send(door_sock, OPEN_EMERG_MSG, strlen(OPEN_EMERG_MSG), 0) == -1) {
        perror("Failed to send open emergency message to door");
        close(door_sock);
        return;
    }

    close(door_sock);
}

int main(int argc, char *argv[]) {
    if(argc != 9) {
        fprintf(stderr, "Usage: %s {address:port} {temperature threshold} {min detections} {detection period (in microseconds)} {reserved argument} {shared memory path} {shared memory offset} {overseer address:port}\n", argv[0]);
        exit(1);
    }

    // Parse command-line arguments
    const char* address = strtok(argv[1], ":");
    int port = atoi(strtok(NULL, ":"));
    float temp_threshold = atof(argv[2]);
    int min_detections = atoi(argv[3]);
    long detection_period = atol(argv[4]);
    const char* shm_path = argv[6];
    int offset = atoi(argv[7]);
    const char* overseer_address = strtok(argv[8], ":");
    int overseer_port = atoi(strtok(NULL, ":"));

    // Open shared memory segment
    int shm_fd = shm_open(shm_path, O_RDWR, 0666);
    if(shm_fd == -1) {
        perror("Failed to open shared memory");
        exit(1);
    }

    // Map shared memory segment into memory
    struct shared_memory *shm_ptr = mmap(NULL, sizeof(struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, offset);
    if(shm_ptr == MAP_FAILED) {
        perror("Failed to map shared memory");
        close(shm_fd);
        exit(1);
    }

    // Bind UDP port
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("Failed to bind UDP port");
        exit(1);
    }

    // Connect to overseer via TCP
    int overseer_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(overseer_sock == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in overseer_addr;
    memset(&overseer_addr, 0, sizeof(overseer_addr));
    overseer_addr.sin_family = AF_INET;
    overseer_addr.sin_port = htons(overseer_port);
    inet_pton(AF_INET, overseer_address, &overseer_addr.sin_addr);

    if(connect(overseer_sock, (struct sockaddr*)&overseer_addr, sizeof(overseer_addr)) == -1) {
        perror("Failed to connect to overseer");
        exit(1);
    }

    // Send initialisation message to overseer
    char init_msg[100];
    snprintf(init_msg, sizeof(init_msg), "FIREALARM %s:%d HELLO#", address, port);
    if(send(overseer_sock, init_msg, strlen(init_msg), 0) == -1) {
        perror("Failed to send initialisation message to overseer");
        exit(1);
    }

    // Main loop
    struct timespec last_detection_time = {0, 0};
    int detection_count = 0;
    struct in_addr door_addrs[100];
    in_port_t door_ports[100];
    int door_count = 0;

    while(1) {
        // Receive next UDP datagram
        struct sockaddr_in source_addr;
        socklen_t source_addr_len = sizeof(source_addr);
        char buffer[1024];
        ssize_t bytes_received = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&source_addr, &source_addr_len);
        if(bytes_received == -1) {
            perror("Failed to receive UDP datagram");
            continue;
        }

        // Handle datagram
        if(memcmp(buffer, "FIRE", 4) == 0) {
            // Fire emergency datagram
            pthread_mutex_lock(&shm_ptr->mutex);
            shm_ptr->alarm = 'A';
            pthread_mutex_unlock(&shm_ptr->mutex);
            pthread_cond_signal(&shm_ptr->cond);

            for(int i = 0; i < door_count; i++) {
                open_emergency_door(door_addrs[i], door_ports[i]);
            }
        } else if(memcmp(buffer, "TEMP", 4) == 0) {
            // Temperature update datagram
            struct temp_datagram* temp_dat = (struct temp_datagram*)buffer;
            float temperature = temp_dat->temperature;
            long timestamp = temp_dat->timestamp;

            if(temperature < temp_threshold) {
                continue;
            }

            struct timespec current_time;
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            long elapsed_time = (current_time.tv_sec - last_detection_time.tv_sec) * 1000000 + (current_time.tv_nsec - last_detection_time.tv_nsec) / 1000;

            if(elapsed_time > detection_period) {
                detection_count = 0;
            }

            last_detection_time = current_time;

            if(detection_count < min_detections) {
                detection_count++;
            } else {
                pthread_mutex_lock(&shm_ptr->mutex);
                shm_ptr->alarm = 'A';
                pthread_mutex_unlock(&shm_ptr->mutex);
                pthread_cond_signal(&shm_ptr->cond);

                for(int i = 0; i < door_count; i++) {
                    open_emergency_door(door_addrs[i], door_ports[i]);
                }
            }
        } else if(memcmp(buffer, "DOOR", 4) == 0) {
            // Door registration datagram
            struct datagram* door_dat = (struct datagram*)buffer;
            struct in_addr door_addr = door_dat->door_addr;
            in_port_t door_port = door_dat->door_port;

            int door_found = 0;
            for(int i = 0; i < door_count; i++) {
                if(memcmp(&door_addrs[i], &door_addr, sizeof(door_addr)) == 0 && door_ports[i] == door_port) {
                    door_found = 1;
                    break;
                }
            }

            if(!door_found) {
                door_addrs[door_count] = door_addr;
                door_ports[door_count] = door_port;
                door_count++;

                send_door_confirmation(overseer_sock, overseer_addr, door_addr, door_port);
            }
        }
    }

    // Cleanup
    munmap(shm_ptr, sizeof(struct shared_memory));
    close(shm_fd);
    close(sock);
    close(overseer_sock);
    return 0;
}
