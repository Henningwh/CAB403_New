
/* Safety-critical component block comment:
This component is safety-critical as it sends emergency signals for fire alarms.
All deviations or exceptions to the standard practice are justified below:
TODO: Add any deviations or exceptions.

Make sure to link with the pthread library (-lpthread) when compiling this program.
*/
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

// Shared memory structure
struct shared_memory {
    char status; // '-' for inactive, '*' for active
    pthread_mutex_t mutex; // Only one process can access status at a time
    pthread_cond_t cond; // Allows threads to wait until status is '*'
};

// Datagram format
struct datagram {
    char header[4]; // {'F', 'I', 'R', 'E'}
};

void send_fire_datagram(const char *addr, int port) {
    struct sockaddr_in destination;
    struct datagram fire = {{'F', 'I', 'R', 'E'}};

    // socket(int family, int type, int protocol)
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    memset(&destination, 0, sizeof(destination));
    destination.sin_family = AF_INET;
    destination.sin_port = htons(port);
    inet_pton(AF_INET, addr, &destination.sin_addr);

    sendto(sock, &fire, sizeof(fire), 0, (struct sockaddr*)&destination, sizeof(destination));
    close(sock);
}

int main(int argc, char *argv[]) {
    if(argc != 5) {
        fprintf(stderr, "Usage: %s <resend delay (in microseconds)> <shared memory path> <shared memory offset> <fire alarm unit address:port>\n", argv[0]);
        exit(1);
    }

    int resend_delay = atoi(argv[1]);
    const char *shm_path = argv[2];
    int offset = atoi(argv[3]);
    char *address = strtok(argv[4], ":");

    int shm_fd = shm_open(shm_path, O_RDWR, 0666);
    if(shm_fd == -1) {
        perror("Failed to open shared memory");
        exit(1);
    }

    struct shared_memory *shm_ptr = mmap(NULL, sizeof(struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, offset);
    if(shm_ptr == MAP_FAILED) {
        perror("Failed to map shared memory");
        close(shm_fd);
        exit(1);
    }

    while(1) {
        pthread_mutex_lock(&shm_ptr->mutex);

        if(shm_ptr->status == '*') {
            send_fire_datagram(address, port);
            usleep(resend_delay);
        } else {
            pthread_cond_wait(&shm_ptr->cond, &shm_ptr->mutex);
        }

        pthread_mutex_unlock(&shm_ptr->mutex);
    }

    // This code should ideally never reach here in normal operation.
    munmap(shm_ptr, sizeof(struct shared_memory));
    close(shm_fd);
    return 0;
}
