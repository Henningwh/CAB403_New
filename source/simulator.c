#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

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
                int door_open_duration, datagram_resend_delay;
                char authorisation_file[64], connections_file[64], layout_file[64];

                sscanf(line, "%*s %*s %d %d %63s %63s %63s", &door_open_duration, &datagram_resend_delay, authorisation_file, connections_file, layout_file);

                int base_offset_for_overseer = (char *)shmPointers.pOverseer - (char *)shmPointers.base;

                int current_offset = base_offset_for_overseer;

                char address_with_port[64];
                snprintf(address_with_port, sizeof(address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

                printf("%s %s %d %d %s %s %s %s %d\n", type, address_with_port, door_open_duration, datagram_resend_delay, authorisation_file, connections_file, layout_file, SHM_PATH, current_offset);
            }
            if (strcmp(type, "firealarm") == 0)
            {
                int temperature_threshold, min_detections, detection_period, reserved_arg;
                sscanf(line, "%*s %*s %d %d %d %d", &temperature_threshold, &min_detections, &detection_period, &reserved_arg);

                int base_offset_for_firealarm = (char *)shmPointers.pFirealarm - (char *)shmPointers.base;
                int current_offset = base_offset_for_firealarm + (counters.firealarm_count * sizeof(shm_firealarm));
                counters.firealarm_count++;

                int local_port = 1234;
                char address_with_local_port[64];
                snprintf(address_with_local_port, sizeof(address_with_local_port), "%s:%d", BASE_ADDRESS, local_port);

                char overseer_address_with_port[64];
                snprintf(overseer_address_with_port, sizeof(overseer_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

                printf("%s %s %d %d %d %s %s %d %s\n", type, address_with_local_port, temperature_threshold, min_detections, detection_period, "reserved", SHM_PATH, current_offset, overseer_address_with_port);
            }
            else if (strcmp(type, "cardreader") == 0)
            {
                int id, waittime;
                sscanf(line, "%*s %*s %d %d", &id, &waittime);

                int base_offset_for_cardreader = (char *)shmPointers.pCardreader - (char *)shmPointers.base;
                int current_offset = base_offset_for_cardreader + (counters.cardreader_count * sizeof(shm_cardreader));
                counters.cardreader_count++;

                char address_with_port[64];
                snprintf(address_with_port, sizeof(address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

                printf("%s %d %d %s %d %s\n", type, id, waittime, SHM_PATH, current_offset, address_with_port);
            }
            else if (strcmp(type, "door") == 0)
            {
                int id;
                char fail_mode[12]; // Enough to store "FAIL_SAFE" or "FAIL_SECURE"

                sscanf(line, "%*s %*s %d %s", &id, fail_mode);

                int base_offset_for_door = (char *)shmPointers.pDoor - (char *)shmPointers.base;
                int current_offset = base_offset_for_door + (counters.door_count * sizeof(shm_door));
                counters.door_count++;

                int local_port = 4321;
                char address_with_local_port[64];
                snprintf(address_with_local_port, sizeof(address_with_local_port), "%s:%d", BASE_ADDRESS, local_port);

                char overseer_address_with_port[64];
                snprintf(overseer_address_with_port, sizeof(overseer_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

                printf("%s %d %s %s %s %d %s\n", type, id, address_with_local_port, fail_mode, SHM_PATH, current_offset, overseer_address_with_port);
            }
            else if (strcmp(type, "callpoint") == 0)
            {
                // Couldnt find where this were called in the scenario files.. ?
                printf("callpoint was called\n");
            }
            else if (strcmp(type, "tempsensor") == 0)
            {
                int id, condvar_wait, update_wait;
                char sensors_data[256]; // To capture remaining data (like S0, S1, S2, etc.)

                sscanf(line, "%*s %*s %d %d %d %[^\n]", &id, &condvar_wait, &update_wait, sensors_data);

                int base_offset_for_tempsensor = (char *)shmPointers.pTempsensor - (char *)shmPointers.base;
                int current_offset = base_offset_for_tempsensor + (counters.tempsensor_count * sizeof(shm_tempsensor));
                counters.tempsensor_count++;

                int local_port = 7272;
                char address_with_local_port[64];
                snprintf(address_with_local_port, sizeof(address_with_local_port), "%s:%d", BASE_ADDRESS, local_port);

                printf("%s %d %s %d %d %s %d %s\n", type, id, address_with_local_port, condvar_wait, update_wait, SHM_PATH, current_offset, sensors_data);
            }
            else if (strcmp(type, "elevator") == 0)
            {
                int id, waittime, open_time;
                sscanf(line, "%*s %*s %d %d %d", &id, &waittime, &open_time);

                int base_offset_for_elevator = (char *)shmPointers.pElevator - (char *)shmPointers.base;
                int current_offset = base_offset_for_elevator + (counters.elevator_count * sizeof(shm_elevator));
                counters.elevator_count++;

                int local_port = 6666;
                char address_with_local_port[64];
                snprintf(address_with_local_port, sizeof(address_with_local_port), "%s:%d", BASE_ADDRESS, local_port);

                char overseer_address_with_port[64];
                snprintf(overseer_address_with_port, sizeof(overseer_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

                printf("%s %d %s %d %d %s %d %s\n", type, id, address_with_local_port, waittime, open_time, SHM_PATH, current_offset, overseer_address_with_port);
            }
            else if (strcmp(type, "destselect") == 0)
            {
                int id, waittime;
                sscanf(line, "%*s %*s %d %d", &id, &waittime);

                int base_offset_for_destselect = (char *)shmPointers.pDestselect - (char *)shmPointers.base;
                int current_offset = base_offset_for_destselect + (counters.destselect_count * sizeof(shm_destselect));
                counters.destselect_count++;

                char overseer_address_with_port[64];
                snprintf(overseer_address_with_port, sizeof(overseer_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

                printf("%s %d %d %s %d %s\n", type, id, waittime, SHM_PATH, current_offset, overseer_address_with_port);
            }
        }
    }

    // Start the overseer component in the foreground
    // ...

    // Simulate events based on the scenario
    // ...

    // Terminate processes and release shared memory
    cleanupSharedMemory();

    fclose(file);
    return 0;
}
