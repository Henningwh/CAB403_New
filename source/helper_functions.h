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

#include "shm_structs.h"

// Enumeration declaration
typedef enum
{
  TYPE_OVERSEER,
  TYPE_FIREALARM,
  TYPE_CARDREADER,
  TYPE_DOOR,
  TYPE_CALLPOINT,
  TYPE_TEMPSENSOR,
  TYPE_ELEVATOR,
  TYPE_DESTSELECT,
  TYPE_UNKNOWN
} ComponentType;

// Function declarations
ComponentType getTypeFromString(const char *str);
void *open_shared_memory(const char *shm_path);
int create_tcp_connection(const char *addr_port_str, int timeout);

#endif
