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

#define MAX_LINES 100
#define LINE_LENGTH 256

// Function declarations
void *open_shared_memory(const char *shm_path);
int create_tcp_connection(const char *addr_port_str, int timeout);
size_t calculateTotalShmSize();
ShmPointers initializeSharedMemory();
void cleanupSharedMemory();
pid_t spawn_process(const char *process_path, char *const argv[]);
void terminate_all_processes(ProcessPIDs *pids);
// Init methods
void initialize_overseer(int current_offset, ShmPointers shmPointers);
void initialize_firealarm(int current_offset, ShmPointers shmPointers);
void initialize_cardreader(int current_offset, ShmPointers shmPointers);
void initialize_door(int current_offset, ShmPointers shmPointers);
void initialize_callpoint(int current_offset, ShmPointers shmPointers);
void initialize_tempsensor(int current_offset, ShmPointers shmPointers);
void initialize_elevator(int current_offset, ShmPointers shmPointers, uint8_t starting_floor);
void initialize_destselect(int current_offset, ShmPointers shmPointers);
void initialize_mutex_cond(void *p, char *component);
void destroy_mutex_cond(instance_counters counters, ShmPointers shmPointers);
void read_file(const char *path, char **init, char **scenario);

#endif
