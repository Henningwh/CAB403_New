#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "data_structs.h"
#include "dg_structs.h"
#include "helper_functions.h"

// Define shared memory path, base address and port
#define SHM_PATH "/shm"
#define BASE_ADDRESS "127.0.0.1"
#define BASE_PORT 3000

instance_counters counters = {0}; // Set them all to 0 as default
ProcessPIDs processPIDs = {0};    // Initialize all PIDs to 0

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

    char *init[MAX_LINES] = {0};
    char *scenario[MAX_LINES] = {0};

    read_file(argv[1], init, scenario);

    printf("--- Initializing Processes: ---\n");
    for (int i = 0; init[i]; i++)
    {
        char component[64];
        sscanf(init[i], "%*s %s", component);

        // {address:port} {door open duration (in microseconds)} {datagram resend delay (in microseconds)} {authorisation file} {connections file} {layout file} {shared memory path} {shared memory offset}
        if (strcmp(component, "overseer") == 0)
        {
            char door_open_duration_str[20], datagram_resend_delay_str[20], authorisation_file[64], connections_file[64], layout_file[64];

            sscanf(init[i], "%*s %*s %19s %19s %63s %63s %63s", door_open_duration_str, datagram_resend_delay_str, authorisation_file, connections_file, layout_file);

            char current_offset_str[20];
            sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pOverseer - (char *)shmPointers.base));

            char address_with_port[64];
            snprintf(address_with_port, sizeof(address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

            printf("%s %s %s %s %s %s %s %s %s\n", component, address_with_port, door_open_duration_str, datagram_resend_delay_str, authorisation_file, connections_file, layout_file, SHM_PATH, current_offset_str);

            char *args[] = {
                "./overseer",
                component,
                address_with_port,
                door_open_duration_str,
                datagram_resend_delay_str,
                authorisation_file,
                connections_file,
                layout_file,
                SHM_PATH,
                current_offset_str,
                NULL};
            initialize_overseer(atoi(current_offset_str), shmPointers);
            processPIDs.overseer = spawn_process("./overseer", args);
            counters.overseer_count++;
            usleep(500000); // Sleep for 0.5sec to let it initiate before continuing
        }
        // {address:port} {temperature threshold} {min detections} {detection period (in microseconds)} {reserved argument} {shared memory path} {shared memory offset} {overseer address:port}
        if (strcmp(component, "firealarm") == 0)
        {
            char temperature_threshold_str[20], min_detections_str[20], detection_period_str[20], reserved_arg_str[20];
            char current_offset_str[20], address_with_local_port[64], overseer_address_with_port[64];
            int local_port_offset = 100;

            sscanf(init[i], "%*s %*s %19s %19s %19s %19s", temperature_threshold_str, min_detections_str, detection_period_str, reserved_arg_str);
            sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pFirealarm - (char *)shmPointers.base + (counters.firealarm_count * sizeof(shm_firealarm))));
            snprintf(address_with_local_port, sizeof(address_with_local_port), "%s:%d", BASE_ADDRESS, BASE_PORT + local_port_offset + counters.firealarm_count);
            snprintf(overseer_address_with_port, sizeof(overseer_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

            printf("%s %s %s %s %s %s %s %s %s\n", component, address_with_local_port, temperature_threshold_str, min_detections_str, detection_period_str, "reserved", SHM_PATH, current_offset_str, overseer_address_with_port);

            char *args[] = {
                component,
                address_with_local_port,
                temperature_threshold_str,
                min_detections_str,
                detection_period_str,
                "reserved",
                SHM_PATH,
                current_offset_str,
                overseer_address_with_port,
                NULL};
            initialize_firealarm(atoi(current_offset_str), shmPointers);
            processPIDs.firealarm = spawn_process("./firealarm", args);
            counters.firealarm_count++;
        }
        // {id} {wait time (in microseconds)} {shared memory path} {shared memory offset} {overseer address:port}
        else if (strcmp(component, "cardreader") == 0)
        {
            char id_str[20], waittime_str[20], current_offset_str[20], address_with_port[64];

            sscanf(init[i], "%*s %*s %19s %19s", id_str, waittime_str);
            sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pCardreader - (char *)shmPointers.base + (counters.cardreader_count * sizeof(shm_cardreader))));
            snprintf(address_with_port, sizeof(address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

            printf("%s %s %s %s %s %s\n", component, id_str, waittime_str, SHM_PATH, current_offset_str, address_with_port);

            char *args[] = {
                component,
                id_str,
                waittime_str,
                SHM_PATH,
                current_offset_str,
                address_with_port,
                NULL};
            initialize_cardreader(atoi(current_offset_str), shmPointers);
            processPIDs.cardreader[counters.cardreader_count] = spawn_process("./cardreader", args);
            counters.cardreader_count++;
        }
        // {id} {address:port} {FAIL_SAFE | FAIL_SECURE} {shared memory path} {shared memory offset} {overseer address:port}
        else if (strcmp(component, "door") == 0)
        {
            char id_str[20], fail_mode[12], current_offset_str[20], address_with_local_port[64], overseer_address_with_port[64];
            int local_port_offset = 300;

            sscanf(init[i], "%*s %*s %19s %11s", id_str, fail_mode);
            sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pDoor - (char *)shmPointers.base + (counters.door_count * sizeof(shm_door))));
            snprintf(address_with_local_port, sizeof(address_with_local_port), "%s:%d", BASE_ADDRESS, BASE_PORT + local_port_offset + counters.door_count);
            snprintf(overseer_address_with_port, sizeof(overseer_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

            printf("%s %s %s %s %s %s %s\n", component, id_str, address_with_local_port, fail_mode, SHM_PATH, current_offset_str, overseer_address_with_port);

            char *args[] = {
                component,
                id_str,
                address_with_local_port,
                fail_mode,
                SHM_PATH,
                current_offset_str,
                overseer_address_with_port,
                NULL};
            initialize_door(atoi(current_offset_str), shmPointers);
            processPIDs.door[counters.door_count] = spawn_process("./door", args);
            counters.door_count++;
        }
        // {resend delay (in microseconds)} {shared memory path} {shared memory offset} {fire alarm unit address:port}
        else if (strcmp(component, "callpoint") == 0)
        {
            // Couldnt find where this were called in the scenario files.. ?
            printf("callpoint was called\n");
        }
        // {id} {address:port} {max condvar wait (microseconds)} {max update wait (microseconds)} {shared memory path} {shared memory offset} {receiver address:port} ...
        else if (strcmp(component, "tempsensor") == 0)
        {
            char id_str[20], condvar_wait_str[20], update_wait_str[20], sensors_data[256], current_offset_str[20], address_with_local_port[64];
            int local_port_offset = 500;

            sscanf(init[i], "%*s %*s %19s %19s %19s %[^\n]", id_str, condvar_wait_str, update_wait_str, sensors_data);
            sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pTempsensor - (char *)shmPointers.base + (counters.tempsensor_count * sizeof(shm_tempsensor))));
            snprintf(address_with_local_port, sizeof(address_with_local_port), "%s:%d", BASE_ADDRESS, BASE_PORT + local_port_offset + counters.tempsensor_count);

            // Set the receiver address and port. Also add "receiver_address_with_port[64]"
            // snprintf(receiver_address_with_port, sizeof(receiver_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

            printf("%s %s %s %s %s %s %s %s\n", component, id_str, address_with_local_port, condvar_wait_str, update_wait_str, SHM_PATH, current_offset_str, sensors_data);

            char *args[] = {
                component,
                id_str,
                address_with_local_port,
                condvar_wait_str,
                update_wait_str,
                SHM_PATH,
                current_offset_str,
                sensors_data,
                NULL};
            initialize_tempsensor(atoi(current_offset_str), shmPointers);
            processPIDs.tempsensor[counters.tempsensor_count] = spawn_process("./tempsensor", args);
            counters.tempsensor_count++;
        }
        // {id} {address:port} {wait time (in microseconds)} {door open time (in microseconds)} {shared memory path} {shared memory offset} {overseer address:port}
        else if (strcmp(component, "elevator") == 0)
        {
            char id_str[20], waittime_str[20], open_time_str[20], current_offset_str[20], address_with_local_port[64], overseer_address_with_port[64], starting_floor[20];
            int local_port_offset = 600;

            sscanf(init[i], "%*s %*s %19s %19s %19s %*s %19s", id_str, waittime_str, open_time_str, starting_floor);
            sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pElevator - (char *)shmPointers.base + (counters.elevator_count * sizeof(shm_elevator))));
            snprintf(address_with_local_port, sizeof(address_with_local_port), "%s:%d", BASE_ADDRESS, BASE_PORT + local_port_offset + counters.elevator_count);
            snprintf(overseer_address_with_port, sizeof(overseer_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

            printf("%s %s %s %s %s %s %s %s\n", component, id_str, address_with_local_port, waittime_str, open_time_str, SHM_PATH, current_offset_str, overseer_address_with_port);

            char *args[] = {
                component,
                id_str,
                address_with_local_port,
                waittime_str,
                open_time_str,
                SHM_PATH,
                current_offset_str,
                overseer_address_with_port,
                NULL};
            initialize_elevator(atoi(current_offset_str), shmPointers, (uint8_t)atoi(starting_floor));
            processPIDs.elevator[counters.elevator_count] = spawn_process("./elevator", args);
            counters.elevator_count++;
        }
        // {id} {wait time (in microseconds)} {shared memory path} {shared memory offset} {overseer address:port}
        else if (strcmp(component, "destselect") == 0)
        {
            char id_str[20], waittime_str[20], current_offset_str[20], overseer_address_with_port[64];
            sscanf(init[i], "%*s %*s %19s %19s", id_str, waittime_str);

            sprintf(current_offset_str, "%d", (int)((char *)shmPointers.pDestselect - (char *)shmPointers.base + (counters.destselect_count * sizeof(shm_destselect))));
            snprintf(overseer_address_with_port, sizeof(overseer_address_with_port), "%s:%d", BASE_ADDRESS, BASE_PORT);

            printf("%s %s %s %s %s %s\n", component, id_str, waittime_str, SHM_PATH, current_offset_str, overseer_address_with_port);

            char *args[] = {
                component,
                id_str,
                waittime_str,
                SHM_PATH,
                current_offset_str,
                overseer_address_with_port,
                NULL};
            initialize_destselect(atoi(current_offset_str), shmPointers);
            processPIDs.destselect[counters.destselect_count] = spawn_process("./destselect", args);
            counters.destselect_count++;
        }
    }

    // Wait for 1 sec, then run scenarios
    usleep(1000000);
    printf("--- SCENARIOS: ---\n");

    for (int i = 0; scenario[i]; i++)
    {
        char timestamp_str[20], num_str[10], event_type[64], code_str[20], new_temperature_str[20], floor_str[10];
        sscanf(scenario[i], "%19s %63s %9s", timestamp_str, event_type, num_str);

        if (strcmp(event_type, "CARD_SCAN") == 0)
        {
            sscanf(scenario[i], "%*s %*s %*s %19s", code_str);
            printf("%s %s %s %s\n", timestamp_str, event_type, num_str, code_str);
        }
        else if (strcmp(event_type, "CALLPOINT_TRIGGER") == 0)
        {
            printf("%s %s %s\n", timestamp_str, event_type, num_str);
        }
        else if (strcmp(event_type, "TEMP_CHANGE") == 0)
        {
            sscanf(scenario[i], "%*s %*s %*s %19s", new_temperature_str);
            printf("%s %s %s %s\n", timestamp_str, event_type, num_str, new_temperature_str);
        }
        else if (strcmp(event_type, "DEST_SELECT") == 0)
        {
            sscanf(scenario[i], "%*s %*s %*s %19s %9s", code_str, floor_str);
            printf("%s %s %s %s %s\n", timestamp_str, event_type, num_str, code_str, floor_str);
        }
        else if (strcmp(event_type, "CAMERA_MOTION") == 0)
        {
            printf("%s %s %s\n", timestamp_str, event_type, num_str);
        }
    }

    // Sleep for 1sec then start the cleanup
    usleep(1000000);

    printf("--- CLEANUP: ---\n");
    destroy_mutex_cond(counters, shmPointers); // Destroy all mutexes and conditions created
    terminate_all_processes(&processPIDs);     // Terminate processes based on PID
    cleanupSharedMemory();                     // Release shared memory
    for (int i = 0; init[i]; i++)              // Free up arrays
    {
        free(init[i]);
    }
    for (int i = 0; scenario[i]; i++)
    {
        free(scenario[i]);
    }

    return 0;
}
