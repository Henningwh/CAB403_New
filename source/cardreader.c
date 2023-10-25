#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>

#include "helper_functions.h"
#include "shm_structs.h"

int main(int argc, char **argv)
{
  if (argc < 6)
  {
    fprintf(stderr, "usage: {id} {wait time (in microseconds)} {shared memory path} {shared memory offset} {overseer address:port}");
    exit(1);
  }

  // Stor all args from commandline to local variables
  int id = atoi(argv[1]);
  int waittime = atoi(argv[2]);
  const char *shm_path = argv[3];
  int shm_offset = atoi(argv[4]);
  const char *overseer_addr = argv[5];

  // Open a shared memory using helper method and cast it to cardreader structure
  void *shm = open_shared_memory(shm_path);
  shm_cardreader *shared = (shm_cardreader *)(shm + shm_offset);

  // Create a socket connection to overseer
  int overseer_fd = create_tcp_connection(overseer_addr, 0);

  // When cardreader is initiated, send "hello" to overseer
  char init_msg[40];
  sprintf(init_msg, "CARDREADER (%d) HELLO#", id);
  if (send(overseer_fd, init_msg, strlen(init_msg), 0) == -1)
  {
    perror("send");
    exit(1);
  }
  // Close the socket connection to overseer
  close(overseer_fd);

  // General cardreader program
  pthread_mutex_lock(&shared->mutex);
  for (;;)
  {
    if (shared->scanned[0] != '\0')
    {
      // Create a socket connection to overseer using the custom waittime from commandline args
      overseer_fd = create_tcp_connection(overseer_addr, waittime);
      char buf[17];
      memcpy(buf, shared->scanned, 16);
      buf[16] = '\0';

      // Send a message to the overseer
      char scanned_msg[100];
      sprintf(scanned_msg, "CARDREADER %d SCANNED %s#", id, buf);
      if (send(overseer_fd, scanned_msg, strlen(scanned_msg), 0) == -1)
      {
        perror("send");
        exit(1);
      }

      char recv_msg[100];
      int len = recv(overseer_fd, recv_msg, 100, 0);
      if (len == -1)
      {
        if (errno == EWOULDBLOCK || errno == EAGAIN || errno == ECONNREFUSED)
        {
          shared->response = 'N';
        }
        else
        {
          perror("recv");
          exit(1);
        }
      }
      // Set the end of the String and set the response to Y/N
      recv_msg[len] = '\0';
      shared->response = (strcmp(recv_msg, "ALLOWED#") == 0) ? 'Y' : 'N';
      pthread_cond_signal(&shared->response_cond);
    }
    // Go back to waiting for cardscans
    pthread_cond_wait(&shared->scanned_cond, &shared->mutex);
  }

  return 0;
}