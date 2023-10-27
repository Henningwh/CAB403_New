/*
  All the shm modules that are going to be used in the project
*/

#ifndef SHM_STRUCTS_H
#define SHM_STRUCTS_H

#include <pthread.h>
#include <stdint.h>

// Overseer
typedef struct
{
  char security_alarm; // '-' if inactive, 'A' if active
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} shm_overseer;

// Firealarm
typedef struct
{
  char alarm; // '-' if inactive, 'A' if active
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} shm_firealarm;

// Cardreader
typedef struct
{
  char scanned[16];
  pthread_mutex_t mutex;
  pthread_cond_t scanned_cond;

  char response; // 'Y' or 'N' (or '\0' at first)
  pthread_cond_t response_cond;
} shm_cardreader;

// Door
typedef struct
{
  char status; // 'O' for open, 'C' for closed, 'o' for opening, 'c' for closing
  pthread_mutex_t mutex;
  pthread_cond_t cond_start;
  pthread_cond_t cond_end;
} shm_door;

// Callpoint
typedef struct
{
  char status; // '-' for inactive, '*' for active
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} shm_callpoint;

// Tempsenor
typedef struct
{
  float temperature;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} shm_tempsensor;

// Elevator
typedef struct
{
  char status;    // 'O' for open, 'C' for closed and not moving, 'o' for opening, 'c' for closing, 'M' for moving
  char direction; // 'U' for up, 'D' for down, '-' for motionless.
  uint8_t floor;
  pthread_mutex_t mutex;
  pthread_cond_t cond_elevator;
  pthread_cond_t cond_controller;
} shm_elevator;

// Destselect
typedef struct
{
  char scanned[16];
  uint8_t floor_select;
  pthread_mutex_t mutex;
  pthread_cond_t scanned_cond;
  char response; // 'Y' or 'N' (or '\0' at first)
  pthread_cond_t response_cond;
} shm_destselect;

typedef struct
{
  uint16_t current_angle;
  uint16_t min_angle;
  uint16_t max_angle;
  char status; // 'L' for turning left, 'R' for turning right, 'O' for on (and stationary), '-' for off
  uint8_t video[36][48];
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} shm_camera;

/* ====================
Structs for overseer
==================== */

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

#endif