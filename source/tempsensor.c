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

  // Last preps before the program starts operating
  float prev_temperature = -100;
  struct timespec last_update_time;
  clock_gettime(CLOCK_REALTIME, &last_update_time);

  // udp socket
  int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

  // //IP UDP Stuff//////////////////////////
  // int listenUDPSocketFD = openAndBindNewUdpPort(overseerUdpPort, moduleName);
  // pthread_t id2;
  // struct RecieveUDPMsgStruct overseerUdpParseAndHandle;
  // overseerUdpParseAndHandle.moduleName = moduleName;
  // overseerUdpParseAndHandle.listenSocketFD = listenUDPSocketFD;
  // overseerUdpParseAndHandle.customParseAndHandleMessage = customParseAndHandleUdp;
  // pthread_create(&id2,NULL,continouslyRecieveUDPMsgAndPrint,(void*)&overseerUdpParseAndHandle);

  while (1)
  {
    pthread_mutex_lock(&p->mutex);
    float current_temperature = p->temperature;
    pthread_mutex_unlock(&p->mutex);

    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    // Check if conditions to send an update are met
    if (current_temperature != prev_temperature ||                                                                                       // Temperature changed
        prev_temperature == -100 ||                                                                                                      // First iteration
        (current_time.tv_sec - last_update_time.tv_sec) * 1e9 + current_time.tv_nsec - last_update_time.tv_nsec > max_update_wait * 1e3) // Max update wait surpassed
    {
      // Construct a UDP datagram
      temp_update_dg datagram;
      // datagram = construct_datagram(id, current_temperature, current_time);

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
          continue;
        }
        dest_addr.sin_port = htons(dist_port);
        inet_pton(AF_INET, dist_address, &dest_addr.sin_addr);

        sendto(udp_socket, &datagram, sizeof(temp_update_dg), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
      }

      last_update_time = current_time; // Update the last update time
    }

    // Wait before restarting the loop struct timespec wait_time;
    // clock_gettime(CLOCK_REALTIME, &wait_time);
    // wait_time.tv_nsec += max_condvar_wait;
    // pthread_cond_timedwait(&p->cond, &p->mutex, &wait_time);
    break;
  }

  return 0;
}