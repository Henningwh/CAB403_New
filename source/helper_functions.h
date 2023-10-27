#ifndef HELPER_FUNCTION_H
#define HELPER_FUNCTION_H

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>

#include "data_structs.h"

// Function declarations
void *open_shared_memory(const char *shm_path);
int create_tcp_connection(const char *addr_port_str, int timeout);
size_t calculateTotalShmSize();
ShmPointers initializeSharedMemory();
void cleanupSharedMemory();

#endif
