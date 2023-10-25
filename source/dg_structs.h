/*
  All the datagram structs that are going to be used in the project
*/

#ifndef DATAGRAMS_H
#define DATAGRAMS_H
#include <sys/time.h>   // for timeval
#include <netinet/in.h> // for door_addr & in_port_t

// Datagram from overseer to fire alarm unit
typedef struct
{
  char header[4]; // {'D', 'O', 'O', 'R'}
  struct in_addr door_addr;
  in_port_t door_port;
} door_reg_dg;

// Datagram from fire alarm unit to overseer
typedef struct
{
  char header[4]; // {'D', 'R', 'E', 'G'}
  struct in_addr door_addr;
  in_port_t door_port;
} door_confirm_dg;

// Datagram to notify the fire alarm of fire
typedef struct
{
  char header[4]; // {'F', 'I', 'R', 'E'}
} fire_emergency_dg;

// Method to add a sensor adress to the sensor struct
struct addr_entry
{
  struct in_addr sensor_addr;
  in_port_t sensor_port;
};

// Datagram sent by a temp sensor to other temp sensors
typedef struct
{
  char header[4]; // {'T', 'E', 'M', 'P'}
  struct timeval timestamp;
  float temperature;
  uint16_t id;
  uint8_t address_count;
  struct addr_entry address_list[50];
} temp_update_dg;

#endif