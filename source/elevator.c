#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "helper_functions.c"
#include "data_structs.h"
#include "ipLibTCP.h"

#define MAX_FLOORS 30
#define MAX_QUEUE_SIZE 2 * MAX_FLOORS

// Define a structure to hold floor requests
typedef struct
{
  int from_floor;
  int to_floor;
} floor_request;

floor_request request_queue[MAX_FLOORS];
int queue_size = 0;

// Mutex for controlling access to the shared memory
pthread_mutex_t shm_mutex = PTHREAD_MUTEX_INITIALIZER;

// Compare a floor input to the queue taking direction into account
int find_insertion_index(int floor, char current_direction)
{
  for (int i = 0; i < queue_size; i++)
  {
    if (current_direction == 'U' && request_queue[i].from_floor > floor)
    {
      return i;
    }
    else if (current_direction == 'D' && request_queue[i].from_floor < floor)
    {
      return i;
    }
  }
  return queue_size;
}

void enqueue_request(int from, int to, shm_elevator *elevator)
{
  // Queue is only full if all buttons are pressed already. Shouldn't be possible.
  if (queue_size >= MAX_QUEUE_SIZE)
  {
    fprintf(stderr, "Queue full!\n");
    return;
  }

  pthread_mutex_lock(&shm_mutex);

  // Check if the floor is already in queue
  int from_index = -1, to_index = -1;
  for (int i = 0; i < queue_size; i++)
  {
    if (request_queue[i].from_floor == from)
      from_index = i;
    if (request_queue[i].to_floor == to)
      to_index = i;
  }

  // Check if the floors already are in the list and in the correct order based on direction
  if (elevator->direction == 'U' && from_index != -1 && to_index != -1 && to_index > from_index)
  {
    pthread_mutex_unlock(&shm_mutex);
    return;
  }
  else if (elevator->direction == 'D' && from_index != -1 && to_index != -1 && to_index < from_index)
  {
    pthread_mutex_unlock(&shm_mutex);
    return;
  }

  // If only the 'from' floor is in the queue, we insert the 'to' floor appropriately
  if (from_index != -1)
  {
    int insert_index = find_insertion_index(to, elevator->direction);
    for (int i = queue_size; i > insert_index; i--) // Shift all elements to the right of the insertion index
    {
      request_queue[i] = request_queue[i - 1];
    }
    request_queue[insert_index].from_floor = from;
    request_queue[insert_index].to_floor = to;
    queue_size++;
  }
  // If only the 'to' floor is in the queue, we insert the 'from' floor appropriately
  else if (to_index != -1)
  {
    int insert_index = find_insertion_index(from, elevator->direction);
    for (int i = queue_size; i > insert_index; i--)
    {
      request_queue[i] = request_queue[i - 1];
    }
    request_queue[insert_index].from_floor = from;
    request_queue[insert_index].to_floor = to;
    queue_size++;
  }
  // If neither floors are in the queue, we insert both floors appropriately
  else
  {
    int insert_index_from = find_insertion_index(from, elevator->direction);
    for (int i = queue_size; i > insert_index_from; i--)
    {
      request_queue[i] = request_queue[i - 1];
    }
    request_queue[insert_index_from].from_floor = from;

    int insert_index_to = find_insertion_index(to, elevator->direction);
    for (int i = queue_size; i > insert_index_to; i--)
    {
      request_queue[i] = request_queue[i - 1];
    }
    request_queue[insert_index_to].to_floor = to;

    queue_size += 2; // since we added both 'from' and 'to'
  }

  pthread_mutex_unlock(&shm_mutex);
}

void process_request(int sockfd, shm_elevator *elevator)
{
  char buffer[256];
  int from, to;

  // Read message from socket
  ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
  if (bytes_received <= 0)
  {
    perror("recv");
    return;
  }
  buffer[bytes_received] = '\0';

  if (sscanf(buffer, "FROM %d TO %d#", &from, &to) != 2)
  {
    fprintf(stderr, "Invalid message format: %s\n", buffer);
    return;
  }

  enqueue_request(from, to, elevator);
}

void elevator_operation(shm_elevator *elevator, int waittime, int door_open_time)
{
  // If queue is empty, exit the method
  if (queue_size == 0)
  {
    return;
  }

  pthread_mutex_lock(&shm_mutex);

  if (elevator->floor == request_queue[0].from_floor && elevator->status == 'C')
  {
    elevator->status = 'o';
    pthread_cond_signal(&elevator->cond_elevator);

    for (int i = 0; i < queue_size - 1; i++)
    {
      request_queue[i] = request_queue[i + 1];
    }
    queue_size--;
  }
  else if (request_queue[0].from_floor > elevator->floor && elevator->status == 'C')
  {
    elevator->direction = 'U';
    pthread_cond_signal(&elevator->cond_elevator);
  }
  else if (request_queue[0].from_floor < elevator->floor && elevator->status == 'C')
  {
    elevator->direction = 'D';
    pthread_cond_signal(&elevator->cond_elevator);
  }
  else if (elevator->status == 'O')
  {
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    long added_nsec_door = timeout.tv_nsec + (door_open_time * 1000);
    timeout.tv_sec += added_nsec_door / 1000000000;
    timeout.tv_nsec = added_nsec_door % 1000000000;
    pthread_cond_timedwait(&elevator->cond_controller, &shm_mutex, &timeout);

    if (elevator->status != 'O')
    {
      pthread_mutex_unlock(&shm_mutex);
      return;
    }

    if (request_queue[0].from_floor > elevator->floor)
      elevator->direction = 'U';
    else if (request_queue[0].from_floor < elevator->floor)
      elevator->direction = 'D';
    else
      elevator->direction = '-';

    elevator->status = 'c';
    pthread_cond_signal(&elevator->cond_elevator);

    clock_gettime(CLOCK_REALTIME, &timeout);
    long added_nsec_wait = timeout.tv_nsec + (waittime * 1000);
    timeout.tv_sec += added_nsec_wait / 1000000000;
    timeout.tv_nsec = added_nsec_wait % 1000000000;
    pthread_cond_timedwait(&elevator->cond_controller, &shm_mutex, &timeout);
  }

  pthread_mutex_unlock(&shm_mutex);

  //Send Hello

    char **resultArrayO = (char **)malloc(10 * sizeof(char *));
    char *inputO = argv[6];
    splitString(inputO, ":", resultArrayO, maxSeqments);
    char* addressO = strdup(resultArrayO[0]);
    int portO = atoi(resultArrayO[1]);
    free(resultArrayO);


    struct CustomSendMsgHandlerAndDependencies sendHelloStruct;
    sendHelloStruct.customMsgHandler = customSendHelloToOverseer;
    sendHelloStruct.remotePort = portO;
    sendHelloStruct.remoteAddr = addressO;
    sendHelloStruct.moduleName = moduleName;
    sendHelloStruct.arguments = argv;

    connectToRemoteSocketAndSendMessage((void*)&sendHelloStruct);


    pthread_join(&id, NULL);

    while(1){}
}


int main(int argc, char *argv[])
{
  if (argc != 8)
  {
    fprintf(stderr, "Usage: %s {id} {address:port} {wait time (in microseconds)} {door open time (in microseconds)} {shared memory path} {shared memory offset} {overseer address:port}\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Initialization: Opening the socket and shared memory
  int sockfd = create_tcp_connection(argv[2], 0); // Assuming no timeout
  shm_elevator *elevator = (shm_elevator *)open_shared_memory(argv[5]);

  int waittime = atoi(argv[3]);
  int door_open_time = atoi(argv[4]);

  // Sending initialization message to overseer
  // TODO: Implement sending of HELLO message

  while (1)
  {
    // TODO: Implement accepting and processing of incoming TCP messages
    process_request(sockfd, elevator);

    // TODO: Implement elevator operation logic

    elevator_operation(elevator, waittime, door_open_time);
  }

  close(sockfd);
  return 0;
}