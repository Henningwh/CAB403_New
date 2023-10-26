#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "shm_structs.h"
#include "dg_structs.h"
#include "helper_functions.h"

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
} shm_master;

// Paths to the different locations in the shared memory
char *shmloc_overseer = "/shm_overseer";
char *shmloc_firealarm = "/shm_firealarm";
char *shmloc_cardreader = "/shm_cardreader";
char *shmloc_door = "/shm_door";
char *shmloc_callpoint = "/shm_callpoint";
char *shmloc_tempsensor = "/shm_tempsensor";
char *shmloc_elevator = "/shm_elevator";
char *shmloc_destselect = "/shm_destselect";

// Helper method to map the shared memory sections
void *initialize_individual_shm(const char *name, size_t size)
{
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        perror("shm_open()");
        exit(1);
    }

    if (ftruncate(fd, size) == -1)
    {
        perror("ftruncate()");
        close(fd);
        exit(1);
    }

    void *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED)
    {
        perror("mmap()");
        close(fd);
        exit(1);
    }

    memset(addr, 0, size); // Set all bytes to 0 -- good practice
    close(fd);
    return addr;
}

// Method to initialize the shared memory
void initializeSharedMemory()
{
    // Master shared memory
    shm_master *master = (shm_master *)initialize_individual_shm("/shm_master", sizeof(shm_master));

    // Now map each struct in shm_master to shared memory and repeat for the max amount of instances there can be
    master->overseer = *(shm_overseer *)initialize_individual_shm(shmloc_overseer, sizeof(shm_overseer));
    master->firealarm = *(shm_firealarm *)initialize_individual_shm(shmloc_firealarm, sizeof(shm_firealarm));

    for (int i = 0; i < 40; i++)
        master->cardreader[i] = *(shm_cardreader *)initialize_individual_shm(shmloc_cardreader, sizeof(shm_cardreader));

    for (int i = 0; i < 20; i++)
    {
        master->door[i] = *(shm_door *)initialize_individual_shm(shmloc_door, sizeof(shm_door));
        master->callpoint[i] = *(shm_callpoint *)initialize_individual_shm(shmloc_callpoint, sizeof(shm_callpoint));
        master->tempsensor[i] = *(shm_tempsensor *)initialize_individual_shm(shmloc_tempsensor, sizeof(shm_tempsensor));
        master->destselect[i] = *(shm_destselect *)initialize_individual_shm(shmloc_destselect, sizeof(shm_destselect));
    }
    for (int i = 0; i < 10; i++)
        master->elevator[i] = *(shm_elevator *)initialize_individual_shm(shmloc_elevator, sizeof(shm_elevator));
}

int main(int argc, char *argv[])
{
    // Check for correct amount of command line arguments
    if (argc != 2)
    {
        printf("Usage: %s <scenario_file>\n", argv[0]);
        return 1;
    }

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
            if (strcmp(type, "cardreader") == 0)
            {
                int id, waittime = {0};
                char path[32], offset[32] = {0};
                char address_string[32] = {0};

                int current_offset = counters.cardreader_count * sizeof(shm_cardreader);
                counters.cardreader_count++;

                snprintf(path, sizeof(path), "%s", shmloc_cardreader);
                snprintf(offset, sizeof(offset), "%d", current_offset);

                sscanf(line, "%*s %*s %d %d", &id, &waittime);
                printf("%s %d %d %s %s %s\n", type, id, waittime, path, offset, address_string);
            }
            else if (strcmp(type, "door") == 0)
            {
                // Code for initializing door
                int id, waittime, offset;
                char path[32], address_string[32];
                sscanf(line, "%*s %*s %d %d %31s %d %31s", &id, &waittime, path, &offset, address_string);
                printf("Door: %d %d %s %d %s\n", id, waittime, path, offset, address_string);
            }
            // For example:
            // else if (strcmp(type, "door") == 0) { ... }
            // else if (strcmp(type, "overseer") == 0) { ... }
            // etc.
        }
    }

    // Initialize shared memory and components
    initializeSharedMemory();

    // Start the overseer component in the foreground
    // ...

    // Simulate events based on the scenario
    // ...

    // Terminate processes and release shared memory
    // ...

    fclose(file);
    return 0;
}
