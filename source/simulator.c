#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// Define shared memory structures for different components
typedef struct
{
    // Define data for the overseer component
    // ...

    pthread_mutex_t overseer_mutex;
    pthread_cond_t overseer_cond;
} OverseerData;

// Define data for other components (fire alarm, card reader, etc.)
// ...

// Initialize shared memory and mutexes
void initializeSharedMemory()
{
    // Initialize shared memory for each component
    // Initialize mutexes and condition variables
    // ...
}

// Simulate an event for a specific component
void simulateEvent(OverseerData *overseer, int componentID)
{
    // Implement the logic for simulating different events
    // You can use mutexes and condition variables to synchronize
    // with the component's behavior.
    // ...
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <scenario_file>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL)
    {
        perror("Failed to open file");
        return 2;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file))
    {
        char command[64], type[64];
        sscanf(line, "%s %s", command, type);

        if (strcmp(command, "INIT") == 0)
        {
            if (strcmp(type, "cardreader") == 0)
            {
                // {id} {wait time (in microseconds)} {shared memory path} {shared memory offset} {overseer address:port}
                // !! Input from simulation is only {id} {wait time (in microseconds)}
                int id, waittime, offset;
                char path[32], adress_string[32];
                sscanf(line, "%*s %*s %d %d %31s %d %31s", &id, &waittime, path, &offset, adress_string); // %*s reads and discards a string
                printf("%s %d %d %s %d %s\n", type, id, waittime, path, offset, adress_string);           // print values read to console
            }
            // Other types can be added here, similar to the above check
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
