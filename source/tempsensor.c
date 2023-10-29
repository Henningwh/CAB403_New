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
    printf("Failed to parse address and port for tempsensor %d.\n", id);
  }
  int max_condvar_wait = atoi(argv[3]);
  int max_update_wait = atoi(argv[4]);
  char *shm_path = argv[5];
  int offset = atoi(argv[6]);
  printf("Hello from tempsensor %d\n", id);

  // Convert argv[7] to their respective address+port
  char line[50];
  strcpy(line, argv[7]);
  char addresses[10][MAX_ADDRESS_LEN];
  int numAddresses = parse_line_to_addresses(line, addresses);

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
    for (int i = 0; i < numAddresses; i++)
    {
      struct sockaddr_in dest_addr;
      memset(&dest_addr, 0, sizeof(dest_addr));
      dest_addr.sin_family = AF_INET;
      char dist_address[40];
      int dist_port = 0;
      if (!parse_address_port(addresses[i], dist_address, &dist_port))
      {
        printf("Failed to parse address and port.\n");
      }
      // dest_addr.sin_port = htons(port);
      // inet_pton(AF_INET, address, &dest_addr.sin_addr);
      //  sendto(udp_socket, &current_temperature, sizeof(current_temperature), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    }

    // Wait before restarting the loop struct timespec wait_time;
    // clock_gettime(CLOCK_REALTIME, &wait_time);
    // wait_time.tv_nsec += max_condvar_wait;
    // pthread_cond_timedwait(&p->cond, &p->mutex, &wait_time);
    break;
  }

  return 0;
}