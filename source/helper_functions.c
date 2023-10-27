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