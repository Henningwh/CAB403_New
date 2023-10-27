#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "shm_structs.h"
#include "dg_structs.h"
#include "helper_functions.h"

// Define shared memory path, base address and port
#define SHM_PATH "/shm"
#define BASE_ADDRESS "127.0.0.1"
#define BASE_PORT 3000

// To keep track of how many of each component we have spawned
typedef struct
{
    int overseer_count;
    int firealarm_count;
    int cardreader_count;
    int door_count;
    int callpoint_count;
    int tempsensor_count;
    int elevator_count;
    int destselect_count;
} instance_counters;
instance_counters counters = {0}; // Set them all to 0 as default

typedef struct
{
    pid_t overseer;
    pid_t firealarm;
    pid_t cardreader[40];
    pid_t door[20];
    pid_t callpoint[20];
    pid_t tempsensor[20];
    pid_t elevator[10];
    pid_t destselect[20];
} ProcessPIDs;
ProcessPIDs processPIDs = {0}; // Initialize all PIDs to 0

// The maximum size the shared memory can be
typedef struct
{
    shm_overseer overseer;
    shm_firealarm firealarm;
    shm_cardreader cardreader[40];
    shm_door door[20];
    shm_callpoint callpoint[20];
    shm_tempsensor tempsensor[20];
    shm_elevator elevator[10];
    shm_destselect destselect[20];
    shm_camera camera[20];
} shm_master;

typedef struct
{
    void *base;
    shm_overseer *pOverseer;
    shm_firealarm *pFirealarm;
    shm_cardreader *pCardreader;
    shm_door *pDoor;
    shm_callpoint *pCallpoint;
    shm_tempsensor *pTempsensor;
    shm_elevator *pElevator;
    shm_destselect *pDestselect;
} ShmPointers;

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

int main(int argc, char *argv[])
{
    // Check for correct amount of command line arguments
    if (argc != 2)
    {
        printf("Usage: %s <scenario_file>\n", argv[0]);
        return 1;
    }

    // Initialize shared memory and components
    cleanupSharedMemory(); // Just in case
    ShmPointers shmPointers = initializeSharedMemory();

    // Open the file supplied in command line argument
    FILE *file = fopen(argv[1], "r");
    if (file == NULL)
    {
        perror("Failed to open file");
        return 2;
    }

    // Read the file
    char line[1024];
    while (fgets(line, sizeof(line), file))
    {
        char command[64], type[64];
        sscanf(line, "%s %s", command, type);

        if (strcmp(command, "INIT") == 0)
        {
            if (strcmp(type, "overseer") == 0)
            {
                char door_open_duration_str[20], datagram_resend_delay_str[20], authorisation_file[64], connections_file[64], layout_file[64];

                sscanf(line, "%*s %*s %19s %19s %63s %63s %63s", door_open_duration_str, datagram_resend_delay_str, authorisation_file, connections_file, layout_file);

                char current_offset_str[20];
                sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pOverseer - (char *)shmPointers.base));

                char address_with_port[64];
                snprintf(address_with_port, sizeof(address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

                printf("%s %s %s %s %s %s %s %s %s\n", type, address_with_port, door_open_duration_str, datagram_resend_delay_str, authorisation_file, connections_file, layout_file, SHM_PATH, current_offset_str);

                char *args[] = {
                    type,
                    address_with_port,
                    door_open_duration_str,
                    datagram_resend_delay_str,
                    authorisation_file,
                    connections_file,
                    layout_file,
                    SHM_PATH,
                    current_offset_str,
                    NULL};
                processPIDs.overseer = spawn_process("./overseer", args);
                counters.overseer_count++;
            }
            if (strcmp(type, "firealarm") == 0)
            {
                char temperature_threshold_str[20], min_detections_str[20], detection_period_str[20], reserved_arg_str[20];
                sscanf(line, "%*s %*s %19s %19s %19s %19s", temperature_threshold_str, min_detections_str, detection_period_str, reserved_arg_str);

                char current_offset_str[20];
                sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pFirealarm - (char *)shmPointers.base + (counters.firealarm_count * sizeof(shm_firealarm))));

                char address_with_local_port[64];
                snprintf(address_with_local_port, sizeof(address_with_local_port), "%s:%d", BASE_ADDRESS, 1234);

                char overseer_address_with_port[64];
                snprintf(overseer_address_with_port, sizeof(overseer_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

                printf("%s %s %s %s %s %s %s %s %s\n", type, address_with_local_port, temperature_threshold_str, min_detections_str, detection_period_str, "reserved", SHM_PATH, current_offset_str, overseer_address_with_port);

                char *args[] = {
                    type,
                    address_with_local_port,
                    temperature_threshold_str,
                    min_detections_str,
                    detection_period_str,
                    "reserved",
                    SHM_PATH,
                    current_offset_str,
                    overseer_address_with_port,
                    NULL};
                processPIDs.firealarm = spawn_process("./firealarm", args);
                counters.firealarm_count++;
            }
            else if (strcmp(type, "cardreader") == 0)
            {
                char id_str[20], waittime_str[20];
                sscanf(line, "%*s %*s %19s %19s", id_str, waittime_str);

                char current_offset_str[20];
                sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pCardreader - (char *)shmPointers.base + (counters.cardreader_count * sizeof(shm_cardreader))));

                char address_with_port[64];
                snprintf(address_with_port, sizeof(address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

                printf("%s %s %s %s %s %s\n", type, id_str, waittime_str, SHM_PATH, current_offset_str, address_with_port);

                char *args[] = {
                    type,
                    id_str,
                    waittime_str,
                    SHM_PATH,
                    current_offset_str,
                    address_with_port,
                    NULL};
                processPIDs.cardreader[counters.cardreader_count] = spawn_process("./cardreader", args);
                counters.cardreader_count++;
            }
            else if (strcmp(type, "door") == 0)
            {
                char id_str[20], fail_mode[12], current_offset_str[20], address_with_local_port[64], overseer_address_with_port[64];
                sscanf(line, "%*s %*s %19s %11s", id_str, fail_mode);

                sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pDoor - (char *)shmPointers.base + (counters.door_count * sizeof(shm_door))));
                snprintf(address_with_local_port, sizeof(address_with_local_port), "%s:%d", BASE_ADDRESS, 4321);
                snprintf(overseer_address_with_port, sizeof(overseer_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

                printf("%s %s %s %s %s %s %s\n", type, id_str, address_with_local_port, fail_mode, SHM_PATH, current_offset_str, overseer_address_with_port);

                char *args[] = {
                    type,
                    id_str,
                    address_with_local_port,
                    fail_mode,
                    SHM_PATH,
                    current_offset_str,
                    overseer_address_with_port,
                    NULL};
                processPIDs.door[counters.door_count] = spawn_process("./door", args);
                counters.door_count++;
            }

            else if (strcmp(type, "callpoint") == 0)
            {
                // Couldnt find where this were called in the scenario files.. ?
                printf("callpoint was called\n");
            }
            else if (strcmp(type, "tempsensor") == 0)
            {
                char id_str[20], condvar_wait_str[20], update_wait_str[20], sensors_data[256], current_offset_str[20], address_with_local_port[64];
                sscanf(line, "%*s %*s %19s %19s %19s %[^\n]", id_str, condvar_wait_str, update_wait_str, sensors_data);

                sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pTempsensor - (char *)shmPointers.base + (counters.tempsensor_count * sizeof(shm_tempsensor))));
                snprintf(address_with_local_port, sizeof(address_with_local_port), "%s:%d", BASE_ADDRESS, 7272);

                printf("%s %s %s %s %s %s %s %s\n", type, id_str, address_with_local_port, condvar_wait_str, update_wait_str, SHM_PATH, current_offset_str, sensors_data);

                char *args[] = {
                    type,
                    id_str,
                    address_with_local_port,
                    condvar_wait_str,
                    update_wait_str,
                    SHM_PATH,
                    current_offset_str,
                    sensors_data,
                    NULL};
                processPIDs.tempsensor[counters.tempsensor_count] = spawn_process("./tempsensor", args);
                counters.tempsensor_count++;
            }
            else if (strcmp(type, "elevator") == 0)
            {
                char id_str[20], waittime_str[20], open_time_str[20], current_offset_str[20], address_with_local_port[64], overseer_address_with_port[64];
                sscanf(line, "%*s %*s %19s %19s %19s", id_str, waittime_str, open_time_str);

                sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pElevator - (char *)shmPointers.base + (counters.elevator_count * sizeof(shm_elevator))));
                snprintf(address_with_local_port, sizeof(address_with_local_port), "%s:%d", BASE_ADDRESS, 6666);
                snprintf(overseer_address_with_port, sizeof(overseer_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

                printf("%s %s %s %s %s %s %s %s\n", type, id_str, address_with_local_port, waittime_str, open_time_str, SHM_PATH, current_offset_str, overseer_address_with_port);

                char *args[] = {
                    type,
                    id_str,
                    address_with_local_port,
                    waittime_str,
                    open_time_str,
                    SHM_PATH,
                    current_offset_str,
                    overseer_address_with_port,
                    NULL};
                processPIDs.elevator[counters.elevator_count] = spawn_process("./elevator", args);
                counters.elevator_count++;
            }
            else if (strcmp(type, "destselect") == 0)
            {
                char id_str[20], waittime_str[20], current_offset_str[20], overseer_address_with_port[64];
                sscanf(line, "%*s %*s %19s %19s", id_str, waittime_str);

                sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pDestselect - (char *)shmPointers.base + (counters.destselect_count * sizeof(shm_destselect))));
                snprintf(overseer_address_with_port, sizeof(overseer_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

                printf("%s %s %s %s %s %s\n", type, id_str, waittime_str, SHM_PATH, current_offset_str, overseer_address_with_port);

                char *args[] = {
                    type,
                    id_str,
                    waittime_str,
                    SHM_PATH,
                    current_offset_str,
                    overseer_address_with_port,
                    NULL};
                processPIDs.destselect[counters.destselect_count] = spawn_process("./destselect", args);
                counters.destselect_count++;
            }
        }
    }

    // Simulate events based on the scenario
    // ...

    // Terminate processes, release shared memory and close the file
    terminate_all_processes(&processPIDs);
    cleanupSharedMemory();
    fclose(file);
    return 0;
}
