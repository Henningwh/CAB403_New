#include "helper_functions.h"

void *open_shared_memory(const char *shm_path)
{
  // From intro video on Canvas
  int shm_fd = shm_open(shm_path, O_RDWR, 0);
  if (shm_fd == -1)
  {
    perror("shm_open");
    close(shm_fd);
    return NULL;
  }
  struct stat shm_stat;
  if (fstat(shm_fd, &shm_stat) == -1)
  {
    perror("fstat");
    close(shm_fd);
    return NULL;
  }
  void *shm = mmap(NULL, shm_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm == MAP_FAILED)
  {
    perror("mmap");
    close(shm_fd);
    return NULL;
  }

  return shm;
}

int create_tcp_connection(const char *full_address, int timeout)
{
  // Separate the port from the adress
  char address[40];
  int port;
  if (sscanf(full_address, "%39[^:]:%d", address, &port) != 2)
  {
    perror("sscanf");
    exit(1);
  }

  // Create socket connection via TCP and store it in a file descriptor
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
  {
    perror("socket");
    exit(1);
  }

  if (timeout > 0)
  {
    struct timeval tv;
    tv.tv_sec = timeout / 1000000;  // Convert to seconds
    tv.tv_usec = timeout % 1000000; // Keep the modulus

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
      perror("setsockopt");
      exit(1);
    }
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_pton(AF_INET, address, &addr.sin_addr) != 1)
  {
    perror("inet_pton");
    close(fd);
    exit(1);
  }

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
  {
    perror("connect");
    close(fd);
    exit(1);
  }

  return fd;
}

size_t calculateTotalShmSize()
{
  return sizeof(shm_overseer) + sizeof(shm_firealarm) + (sizeof(shm_cardreader) * 40) +
         (sizeof(shm_door) * 20) + (sizeof(shm_callpoint) * 20) + (sizeof(shm_tempsensor) * 20) +
         (sizeof(shm_elevator) * 10) + (sizeof(shm_destselect) * 20) + (sizeof(shm_camera) * 20);
}

ShmPointers initializeSharedMemory()
{
  ShmPointers pointers;
  size_t offset = 0;

  // Initialize shared memory space
  int fd = shm_open("/shm", O_CREAT | O_RDWR, 0);
  if (fd == -1)
  {
    perror("Error: An error occurred in shm_open()");
    exit(1);
  }

  size_t size = calculateTotalShmSize();
  printf("Total shared memory size: %zu bytes (Should be 54592)\n", size);
  if (ftruncate(fd, size) == -1)
  {
    perror("Error: An error occurred in ftruncate()");
    close(fd);
    exit(1);
  }

  void *master = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (master == MAP_FAILED)
  {
    perror("Error: An error occurred in mmap()");
    close(fd);
    exit(1);
  }
  memset(master, 0, size);
  close(fd);

  // Base
  pointers.base = master;
  // Overseer
  pointers.pOverseer = (shm_overseer *)((char *)master + offset);
  // Firealarm
  offset += sizeof(shm_overseer);
  pointers.pFirealarm = (shm_firealarm *)((char *)master + offset);
  // Cardreader
  offset += sizeof(shm_firealarm);
  pointers.pCardreader = (shm_cardreader *)((char *)master + offset);
  // Door
  offset += sizeof(shm_cardreader) * 40;
  pointers.pDoor = (shm_door *)((char *)master + offset);
  // Callpoint
  offset += sizeof(shm_door) * 20;
  pointers.pCallpoint = (shm_callpoint *)((char *)master + offset);
  // Tempsensor
  offset += sizeof(shm_callpoint) * 20;
  pointers.pTempsensor = (shm_tempsensor *)((char *)master + offset);
  // Elevator
  offset += sizeof(shm_tempsensor) * 20;
  pointers.pElevator = (shm_elevator *)((char *)master + offset);
  // Destselect
  offset += sizeof(shm_elevator) * 10;
  pointers.pDestselect = (shm_destselect *)((char *)master + offset);

  return pointers;
}

void cleanupSharedMemory()
{
  shm_unlink("/shm");
}

pid_t spawn_process(const char *process_path, char *const argv[])
{
  pid_t processPID = fork();

  if (processPID == 0)
  {
    // Child process
    usleep(250000);
    execv(process_path, argv);
    // If execv() fails:
    perror("Error: execv() failed to launch process");
    exit(1);
  }
  else if (processPID < 0)
  {
    perror("Error: fork() failed");
    exit(1);
  }
  // Parent process continues here:
  return processPID;
}

void terminate_all_processes(ProcessPIDs *pids)
{
  int i;

  if (pids->overseer)
  {
    printf("Killing process: %d\n", pids->overseer);
    kill(pids->overseer, SIGTERM);
  }
  if (pids->firealarm)
  {
    printf("Killing process: %d\n", pids->firealarm);
    kill(pids->firealarm, SIGTERM);
  }
  for (i = 0; i < 40; i++)
  {
    if (pids->cardreader[i])
    {
      printf("Killing process: %d\n", pids->cardreader[i]);
      kill(pids->cardreader[i], SIGTERM);
    }
  }
  for (i = 0; i < 20; i++)
  {
    if (pids->door[i])
    {
      printf("Killing process: %d\n", pids->door[i]);
      kill(pids->door[i], SIGTERM);
    }
    if (pids->callpoint[i])
    {
      printf("Killing process: %d\n", pids->callpoint[i]);
      kill(pids->callpoint[i], SIGTERM);
    }
    if (pids->tempsensor[i])
    {
      printf("Killing process: %d\n", pids->tempsensor[i]);
      kill(pids->tempsensor[i], SIGTERM);
    }
    if (pids->destselect[i])
    {
      printf("Killing process: %d\n", pids->destselect[i]);
      kill(pids->destselect[i], SIGTERM);
    }
  }
  for (i = 0; i < 10; i++)
  {
    if (pids->elevator[i])
    {
      printf("Killing process: %d\n", pids->elevator[i]);
      kill(pids->elevator[i], SIGTERM);
    }
  }
}

/*
Initialization of components
*/
void initialize_overseer(int current_offset, ShmPointers shmPointers)
{
  shm_overseer *p = (shm_overseer *)((char *)shmPointers.base + current_offset);

  p->security_alarm = '-';
}

void initialize_firealarm(int current_offset, ShmPointers shmPointers)
{
  shm_firealarm *p = (shm_firealarm *)((char *)shmPointers.base + current_offset);

  p->alarm = '-';
}

void initialize_cardreader(int current_offset, ShmPointers shmPointers)
{
  shm_cardreader *p = (shm_cardreader *)((char *)shmPointers.base + current_offset);

  memset(p->scanned, '\0', sizeof(p->scanned));
  p->response = '\0';
}

void initialize_door(int current_offset, ShmPointers shmPointers)
{
  shm_door *p = (shm_door *)((char *)shmPointers.base + current_offset);

  p->status = 'C';
}

void initialize_callpoint(int current_offset, ShmPointers shmPointers)
{
  shm_callpoint *p = (shm_callpoint *)((char *)shmPointers.base + current_offset);

  p->status = '-';
}

void initialize_tempsensor(int current_offset, ShmPointers shmPointers)
{
  shm_tempsensor *p = (shm_tempsensor *)((char *)shmPointers.base + current_offset);

  p->temperature = 22.0f;
}

void initialize_elevator(int current_offset, ShmPointers shmPointers, char starting_floor)
{
  shm_elevator *p = (shm_elevator *)((char *)shmPointers.base + current_offset);

  p->status = 'C';
  p->direction = '-';
  p->floor = starting_floor;
}
void initialize_destselect(int current_offset, ShmPointers shmPointers)
{
  shm_destselect *p = (shm_destselect *)((char *)shmPointers.base + current_offset);

  memset(p->scanned, '\0', sizeof(p->scanned));
  p->response = '\0';
  p->floor_select = 0;
}