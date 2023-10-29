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

#include "ipLibUDP.h"
#include "helper_functions.h"
#include "data_structs.h"
#include "dg_structs.h"

#define MAX_RECEIVERS 50
temp_update_dg local_dg;

void customParseAndHandleUdp(temp_update_dg *msg)
{
  printf("%d CHECK\n", local_dg.id);
  free(msg);
}

int main(int argc, char **argv)
{
  if (argc < 8) // Check for correct amount of command line args
  {
    fprintf(stderr, "Usage: {id} {address:port} {max condvar wait (microseconds)} {max update wait (microseconds)} {shared memory path} {shared memory offset} {receiver address:port} ...\n");
    exit(1);
  }
  // Extract command line parameters
  int id = atoi(argv[1]);
  local_dg.id = id;
  local_dg.address_count = 1;
  char address[40];
  int port;

  if (!parse_address_port(argv[2], address, &port))
  {
    printf("Failed to parse address and port for tempsensor %d.\n", id);
  }
  // int max_condvar_wait = atoi(argv[3]);
  int max_update_wait = atoi(argv[4]);
  char *shm_path = argv[5];
  int offset = atoi(argv[6]);

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

  // UDP Listener on a spesific port
  char moduleName[50];
  sprintf(moduleName, "Tempsensor%d", id);
  int listenUDPSocketFD = openAndBindNewUdpPort(port, moduleName);
  pthread_t id2;
  struct RecieveUDPMsgStruct tempsensorUdpParseAndHandle;
  tempsensorUdpParseAndHandle.moduleName = moduleName;
  tempsensorUdpParseAndHandle.listenSocketFD = listenUDPSocketFD;
  tempsensorUdpParseAndHandle.customParseAndHandleMessage = customParseAndHandleUdp;
  pthread_create(&id2, NULL, continouslyRecieveUDPMsgAndPrintTemp, (void *)&tempsensorUdpParseAndHandle);

  while (1)
  {
    pthread_mutex_lock(&p->mutex);
    float current_temperature = p->temperature;
    local_dg.temperature = current_temperature;
    pthread_mutex_unlock(&p->mutex);

    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    // Check if conditions to send an update are met
    if (current_temperature != prev_temperature ||                                                                                       // Temperature changed
        prev_temperature == -100 ||                                                                                                      // First iteration
        (current_time.tv_sec - last_update_time.tv_sec) * 1e9 + current_time.tv_nsec - last_update_time.tv_nsec > max_update_wait * 1e3) // Max update wait surpassed
    {
      // Construct a UDP datagram
      // temp_update_dg datagram;
      // datagram = construct_datagram(id, current_temperature, current_time);

      for (int i = 0; i < numAddresses; i++)
      {
        char dist_address[10];
        int dist_port = 0;
        if (!parse_address_port(addresses[i], dist_address, &dist_port))
        {
          printf("Failed to parse address and port.\n");
          continue;
        }

        strcpy(local_dg.header, "TEMP");
        sendUDPMessageTemp(&local_dg, dist_address, dist_port, moduleName);
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