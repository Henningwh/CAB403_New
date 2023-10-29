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

#include "helper_functions.h"
#include "data_structs.h"
#include "dg_structs.h"

#define MAX_RECEIVERS 50

int main(int argc, char **argv)
{
  if (argc < 8) // Check for correct amount of command line args
  {
    fprintf(stderr, "Usage: {id} {address:port} {max condvar wait (microseconds)} {max update wait (microseconds)} {shared memory path} {shared memory offset} {receiver address:port} ...\n");
    exit(1);
  }
  // Extract command line parameters
  int id = atoi(argv[1]);
  char address[40];
  int port;

  if (!parse_address_port(argv[2], address, &port))
  {
    printf("Failed to parse address and port.\n");
  }
  int max_condvar_wait = atoi(argv[3]);
  int max_update_wait = atoi(argv[4]);
  char *shm_path = argv[5];
  int offset = atoi(argv[6]);
  printf("INITIALIZE: Tempsensor with id '%d' using path '%s' with offset %d\n", id, shm_path, offset);

  char *receivers[MAX_RECEIVERS];
  int num_receivers = argc - 7; // Recievers are all args after the first 7
  for (int i = 0; i < num_receivers; i++)
  {
    receivers[i] = argv[i + 7];
  }

  // Shared memory setup
  void *base = open_shared_memory(shm_path);
  shm_tempsensor *p = (shm_tempsensor *)((char *)base + offset);

  // UDP socket setup

  while (1)
  {
    // Read temp
    pthread_mutex_lock(&p->mutex);
    float current_temperature = p->temperature;
    pthread_mutex_unlock(&p->mutex);

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
        printf("Couldn't fetch the address and port from %s\n", receivers[i]);
        exit(1);
      }
      dest_addr.sin_port = htons(port);
      inet_pton(AF_INET, address, &dest_addr.sin_addr);
      //  sendto(udp_socket, &current_temperature, sizeof(current_temperature), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    }

    // Wait before restarting the loop
    struct timespec wait_time;
    clock_gettime(CLOCK_REALTIME, &wait_time);
    wait_time.tv_nsec += max_condvar_wait;
    pthread_cond_timedwait(&p->cond, &p->mutex, &wait_time);
  }

  return 0;
}