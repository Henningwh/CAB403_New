#include "helper_functions.h"

void *open_shared_memory(const char *shm_path)
{
  // From intro video on Canvas
  int fd = shm_open(shm_path, O_RDWR, 0);
  if (fd == -1)
  {
    perror("Error: An error occurred in shm_open()");
    close(fd);
    return 0;
  }
  size_t size = calculateTotalShmSize();
  void *base = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (base == MAP_FAILED)
  {
    perror("Error: An error occurred in mmap()");
    close(fd);
    return 0;
  }
  close(fd);
  return base;
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
  int fd = shm_open("/shm", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
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

  initialize_mutex_cond(p, "overseer");
  p->security_alarm = '-';
}

void initialize_firealarm(int current_offset, ShmPointers shmPointers)
{
  shm_firealarm *p = (shm_firealarm *)((char *)shmPointers.base + current_offset);

  initialize_mutex_cond(p, "firealarm");
  p->alarm = '-';
}

void initialize_cardreader(int current_offset, ShmPointers shmPointers)
{
  shm_cardreader *p = (shm_cardreader *)((char *)shmPointers.base + current_offset);

  initialize_mutex_cond(p, "cardreader");
  memset(p->scanned, '\0', sizeof(p->scanned));
  p->response = '\0';
}

void initialize_door(int current_offset, ShmPointers shmPointers)
{
  shm_door *p = (shm_door *)((char *)shmPointers.base + current_offset);

  initialize_mutex_cond(p, "door");
  p->status = 'C';
}

void initialize_callpoint(int current_offset, ShmPointers shmPointers)
{
  shm_callpoint *p = (shm_callpoint *)((char *)shmPointers.base + current_offset);

  initialize_mutex_cond(p, "callpoint");
  p->status = '-';
}

void initialize_tempsensor(int current_offset, ShmPointers shmPointers)
{
  shm_tempsensor *p = (shm_tempsensor *)((char *)shmPointers.base + current_offset);

  initialize_mutex_cond(p, "tempsensor");
  p->temperature = 22.0f;
}

void initialize_elevator(int current_offset, ShmPointers shmPointers, uint8_t starting_floor)
{
  shm_elevator *p = (shm_elevator *)((char *)shmPointers.base + current_offset);

  initialize_mutex_cond(p, "elevator");
  p->status = 'C';
  p->direction = '-';
  p->floor = starting_floor;
}
void initialize_destselect(int current_offset, ShmPointers shmPointers)
{
  shm_destselect *p = (shm_destselect *)((char *)shmPointers.base + current_offset);

  initialize_mutex_cond(p, "destselect");
  memset(p->scanned, '\0', sizeof(p->scanned));
  p->response = '\0';
  p->floor_select = 0;
}

void initialize_mutex_cond(void *p, char *component)
{
  pthread_mutexattr_t mattr;
  pthread_condattr_t cattr;

  // Init mutex attributes
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);

  // Init cond attributes
  pthread_condattr_init(&cattr);
  pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

  if (strcmp(component, "overseer") == 0)
  {
    shm_overseer *overseer = (shm_overseer *)p;
    pthread_mutex_init(&overseer->mutex, &mattr);
    pthread_cond_init(&overseer->cond, &cattr);
  }
  else if (strcmp(component, "firealarm") == 0)
  {
    shm_firealarm *firealarm = (shm_firealarm *)p;
    pthread_mutex_init(&firealarm->mutex, &mattr);
    pthread_cond_init(&firealarm->cond, &cattr);
  }
  else if (strcmp(component, "cardreader") == 0)
  {
    shm_cardreader *cardreader = (shm_cardreader *)p;
    pthread_mutex_init(&cardreader->mutex, &mattr);
    pthread_cond_init(&cardreader->scanned_cond, &cattr);
    pthread_cond_init(&cardreader->response_cond, &cattr);
  }
  else if (strcmp(component, "door") == 0)
  {
    shm_door *door = (shm_door *)p;
    pthread_mutex_init(&door->mutex, &mattr);
    pthread_cond_init(&door->cond_start, &cattr);
    pthread_cond_init(&door->cond_end, &cattr);
  }
  else if (strcmp(component, "callpoint") == 0)
  {
    shm_callpoint *callpoint = (shm_callpoint *)p;
    pthread_mutex_init(&callpoint->mutex, &mattr);
    pthread_cond_init(&callpoint->cond, &cattr);
  }
  else if (strcmp(component, "tempsensor") == 0)
  {
    shm_tempsensor *tempsensor = (shm_tempsensor *)p;
    pthread_mutex_init(&tempsensor->mutex, &mattr);
    pthread_cond_init(&tempsensor->cond, &cattr);
  }
  else if (strcmp(component, "elevator") == 0)
  {
    shm_elevator *elevator = (shm_elevator *)p;
    pthread_mutex_init(&elevator->mutex, &mattr);
    pthread_cond_init(&elevator->cond_elevator, &cattr);
    pthread_cond_init(&elevator->cond_controller, &cattr);
  }
  else if (strcmp(component, "destselect") == 0)
  {
    shm_destselect *destselect = (shm_destselect *)p;
    pthread_mutex_init(&destselect->mutex, &mattr);
    pthread_cond_init(&destselect->scanned_cond, &cattr);
    pthread_cond_init(&destselect->response_cond, &cattr);
  }
  else
  {
    printf("Couldnt initialize component named '%s'", component);
  }
}

void destroy_mutex_cond(instance_counters counters, ShmPointers shmPointers)
{
  if (counters.overseer_count > 0)
  {
    shm_overseer *overseer = (shm_overseer *)shmPointers.pOverseer;
    pthread_mutex_destroy(&overseer->mutex);
    pthread_cond_destroy(&overseer->cond);
  }

  if (counters.firealarm_count > 0)
  {
    shm_firealarm *firealarm = (shm_firealarm *)shmPointers.pFirealarm;
    pthread_mutex_destroy(&firealarm->mutex);
    pthread_cond_destroy(&firealarm->cond);
  }

  for (int i = 0; i < counters.cardreader_count; ++i)
  {
    void *current = (char *)shmPointers.pCardreader + i * sizeof(shm_cardreader);
    shm_cardreader *cardreader = (shm_cardreader *)current;
    pthread_mutex_destroy(&cardreader->mutex);
    pthread_cond_destroy(&cardreader->scanned_cond);
    pthread_cond_destroy(&cardreader->response_cond);
  }

  for (int i = 0; i < counters.door_count; ++i)
  {
    void *current = (char *)shmPointers.pDoor + i * sizeof(shm_door);
    shm_door *door = (shm_door *)current;
    pthread_mutex_destroy(&door->mutex);
    pthread_cond_destroy(&door->cond_start);
    pthread_cond_destroy(&door->cond_end);
  }

  for (int i = 0; i < counters.callpoint_count; ++i)
  {
    void *current = (char *)shmPointers.pCallpoint + i * sizeof(shm_callpoint);
    shm_callpoint *callpoint = (shm_callpoint *)current;
    pthread_mutex_destroy(&callpoint->mutex);
    pthread_cond_destroy(&callpoint->cond);
  }

  for (int i = 0; i < counters.tempsensor_count; ++i)
  {
    void *current = (char *)shmPointers.pTempsensor + i * sizeof(shm_tempsensor);
    shm_tempsensor *tempsensor = (shm_tempsensor *)current;
    pthread_mutex_destroy(&tempsensor->mutex);
    pthread_cond_destroy(&tempsensor->cond);
  }

  for (int i = 0; i < counters.elevator_count; ++i)
  {
    void *current = (char *)shmPointers.pElevator + i * sizeof(shm_elevator);
    shm_elevator *elevator = (shm_elevator *)current;
    pthread_mutex_destroy(&elevator->mutex);
    pthread_cond_destroy(&elevator->cond_elevator);
    pthread_cond_destroy(&elevator->cond_controller);
  }

  for (int i = 0; i < counters.destselect_count; ++i)
  {
    void *current = (char *)shmPointers.pDestselect + i * sizeof(shm_destselect);
    shm_destselect *destselect = (shm_destselect *)current;
    pthread_mutex_destroy(&destselect->mutex);
    pthread_cond_destroy(&destselect->scanned_cond);
    pthread_cond_destroy(&destselect->response_cond);
  }
}

void read_file(const char *path, char **init, char **scenario)
{
  FILE *file = fopen(path, "r");
  if (!file)
  {
    perror("Failed to open file");
    return;
  }

  char buffer[LINE_LENGTH];
  int init_index = 0;
  int scenario_index = 0;
  int in_scenario = 0;

  while (fgets(buffer, LINE_LENGTH, file))
  {
    buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

    if (!in_scenario)
    {
      if (strcmp(buffer, "SCENARIO") == 0)
      {
        in_scenario = 1;
        continue;
      }
      init[init_index] = strdup(buffer);
      init_index++;
    }
    else
    {
      scenario[scenario_index] = strdup(buffer);
      scenario_index++;
    }
  }

  fclose(file);
}

bool parse_address_port(const char *input, char *address, int *port)
{
  int result = sscanf(input, "%39[^:]:%d", address, port);
  return result == 2;
}

int get_port_from_code(const char *code)
{
  if (strcmp(code, "O") == 0)
  {
    return 5000;
  }
  else if (strcmp(code, "F") == 0)
  {
    return 3100;
  }
  else if (code[0] == 'S')
  {
    return 3500 + atoi(code + 1);
  }
  return -1; // error
}

int parse_line_to_addresses(const char *line, char result[][MAX_ADDRESS_LEN])
{
  int count = 0;
  char *token = strtok((char *)line, " ");

  while (token != NULL)
  {
    int port = get_port_from_code(token);
    if (port != -1)
    {
      snprintf(result[count], MAX_ADDRESS_LEN, "127.0.0.1:%d", port);
      count++;
    }
    token = strtok(NULL, " ");
  }

  return count;
}