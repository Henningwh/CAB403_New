#ifndef HELPER_FUNCTION_H
#define HELPER_FUNCTION_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

#include "data_structs.h"

// Function declarations
void *open_shared_memory(const char *shm_path);
int create_tcp_connection(const char *addr_port_str, int timeout);
size_t calculateTotalShmSize();
ShmPointers initializeSharedMemory();
void cleanupSharedMemory();
pid_t spawn_process(const char *process_path, char *const argv[]);
void terminate_all_processes(ProcessPIDs *pids);

#endif
