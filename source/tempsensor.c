#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "helper_functions.c"
#include "shm_structs.h"
#include "dg_structs.h"

#define MAX_RECEIVERS 50

int main(int argc, char **argv)
{
  // Check for correct amount of command line args
  if (argc < 8)
  {
    fprintf(stderr, "Insufficient arguments provided\n");
    exit(1);
  }

  // Extract command line parameters
  int id = atoi(argv[1]);
  char *local_address_port = argv[2];
  int max_condvar_wait = atoi(argv[3]);
  int max_update_wait = atoi(argv[4]);
  char *shm_path = argv[5];
  int shm_offset = atoi(argv[6]);

  char *receivers[MAX_RECEIVERS];
  int num_receivers = argc - 7; // Recievers are all args after the first 7
  for (int i = 0; i < num_receivers; i++)
  {
    receivers[i] = argv[i + 7];
  }

  // Shared memory setup and set up a UDP socket
  shm_tempsensor *shm = (shm_tempsensor *)(open_shared_memory(shm_path) + shm_offset);
  struct sockaddr_in local_addr;
  int local_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (local_socket == -1)
  {
    perror("socket()");
    exit(1);
  }
  memset(&local_addr, 0, sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(atoi(local_address_port));
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(local_socket, (struct sockaddr *)&local_addr, sizeof(local_addr));

  while (1)
  {
    // Read temp
    pthread_mutex_lock(&shm->mutex);
    float current_temperature = shm->temperature;
    pthread_mutex_unlock(&shm->mutex);

    // Distribute the temp to all recievers specified in the command line args
    for (int i = 0; i < num_receivers; i++)
    {
      struct sockaddr_in dest_addr;
      memset(&dest_addr, 0, sizeof(dest_addr));
      dest_addr.sin_family = AF_INET;
      char address[40];
      int port;
      if (sscanf(receivers[i], "%39[^:]:%d", address, &port) != 2)
      {
        perror("sscanf");
        exit(1);
      }
      dest_addr.sin_port = htons(port);
      inet_pton(AF_INET, address, &dest_addr.sin_addr);
      sendto(local_socket, &current_temperature, sizeof(current_temperature), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    }

    // Wait before restarting the loop
    struct timespec wait_time;
    clock_gettime(CLOCK_REALTIME, &wait_time);
    wait_time.tv_nsec += max_condvar_wait;
    pthread_cond_timedwait(&shm->cond, &shm->mutex, &wait_time);
  }

  return 0;
}