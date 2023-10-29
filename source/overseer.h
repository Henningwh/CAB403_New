#ifndef overseer_H
#define overseer_H

struct CardReaderInfo{
    int id;
};

struct DoorInfo {
    int id;
    char* ip;
    int port;
    char* configuration;
};

struct FireAlarmInfo{
    int id;
    char* ip;
    int port;
};

struct ElevatorInfo{
    int id;
    char* ip;
    int port;
};

struct DestSelectInfo{
    int id;
};

struct DoorInfoGrouped{
    struct DoorInfo failSecure[10];
    struct DoorInfo failSafe[10];
};

void overseer();

#endif