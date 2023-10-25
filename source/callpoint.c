
/* Safety-critical component block comment:
This component is safety-critical as it sends emergency signals for fire alarms.
All deviations or exceptions to the standard practice are justified below:

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

#include "helper_functions.h"
#include "shm_structs.h"
#include "dg_structs.h"

void send_fire_datagram(int sock, const struct sockaddr_in *destination)
{
  fire_emergency_dg fire = {{'F', 'I', 'R', 'E'}};
  sendto(sock, &fire, sizeof(fire), 0, (struct sockaddr *)destination, sizeof(*destination));
}

int main(int argc, char *argv[])
{
  // Check that there are correct amount of args
  if (argc != 5)
  {
    fprintf(stderr, "Usage: %s <resend delay (in microseconds)> <shared memory path> <shared memory offset> <fire alarm unit address:port>\n", argv[0]);
    exit(1);
  }

  // Save the command line args to local variables
  int resend_delay = atoi(argv[1]);
  const char *shm_path = argv[2];
  int offset = atoi(argv[3]);

  // Open shared memory using helper_functions
  char *shm = open_shared_memory(shm_path);
  shm_callpoint *shared = (shm_callpoint *)(shm + offset);

  // Split the port from the address and save both in their own variable
  char addr_str[100], *port_str;
  strcpy(addr_str, argv[4]);
  port_str = strchr(addr_str, ':');
  *port_str++ = '\0';
  int port = atoi(port_str);

  // Create a UDP socket
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in destination = {0};
  destination.sin_family = AF_INET;
  destination.sin_port = htons(port);
  inet_pton(AF_INET, addr_str, &destination.sin_addr);

  while (1)
  {
    pthread_mutex_lock(&shared->mutex);
    if (shared->status == '*')
    {
      send_fire_datagram(sock, &destination);
      usleep(resend_delay);
    }
    else
    {
      pthread_cond_wait(&shared->cond, &shared->mutex);
    }
    pthread_mutex_unlock(&shared->mutex);
  }
  close(sock);
  return 0;
}