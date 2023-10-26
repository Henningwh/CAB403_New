#include "helper_functions.h"

ComponentType getTypeFromString(const char *str)
{
  if (strcmp(str, "overseer") == 0)
    return TYPE_OVERSEER;
  if (strcmp(str, "firealarm") == 0)
    return TYPE_FIREALARM;
  if (strcmp(str, "cardreader") == 0)
    return TYPE_CARDREADER;
  if (strcmp(str, "door") == 0)
    return TYPE_DOOR;
  if (strcmp(str, "callpoint") == 0)
    return TYPE_CALLPOINT;
  if (strcmp(str, "tempsensor") == 0)
    return TYPE_TEMPSENSOR;
  if (strcmp(str, "elevator") == 0)
    return TYPE_ELEVATOR;
  if (strcmp(str, "destselect") == 0)
    return TYPE_DESTSELECT;
  return TYPE_UNKNOWN;
}

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