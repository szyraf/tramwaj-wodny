#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <ctime>

#define MAX_PASSENGERS 10000
#define MAX_BRIDGE 10000

#define IPC_KEY_BASE 0x1234

#define SHM_KEY (IPC_KEY_BASE + 1)
#define SEM_KEY (IPC_KEY_BASE + 2)
#define MSG_KEY (IPC_KEY_BASE + 3)

enum Phase {
    PHASE_INIT = 0,
    PHASE_LOADING = 1,
    PHASE_BRIDGE_CLEAR = 2,
    PHASE_SAILING = 3,
    PHASE_UNLOADING = 4,
    PHASE_END = 5
};

enum Location {
    TYNIEC = 0,
    WAWEL = 1
};

enum PassengerState {
    STATE_QUEUE = 0,
    STATE_BRIDGE = 1,
    STATE_SHIP = 2,
    STATE_EXITED = 3
};

enum SemIndex {
    SEM_MUTEX = 0,
    SEM_CAPTAIN_READY = 1,
    SEM_PASSENGER_BASE = 2
};

struct MsgBuf {
    long mtype;
    int data;
};

#define MSG_ACK 1
#define MSG_READY 2

struct SharedState {
    Phase phase;
    Location ship_location;
    int trip_num;
    int max_trips;
    
    int ship_people;
    int ship_bikes;
    int ship_capacity_people;
    int ship_capacity_bikes;
    
    int bridge_count;
    int bridge_bike_slots;
    int bridge_capacity;
    
    bool signal1;
    bool signal2;
    bool day_ended;
    bool loading_done;
    
    int passenger_count;
    int passenger_state[MAX_PASSENGERS];
    int passenger_location[MAX_PASSENGERS];
    bool passenger_has_bike[MAX_PASSENGERS];
    int passenger_queue_pos[MAX_PASSENGERS];
    
    int queue_tyniec[MAX_PASSENGERS];
    int queue_tyniec_size;
    int queue_wawel[MAX_PASSENGERS];
    int queue_wawel_size;
    
    int bridge_queue[MAX_BRIDGE];
    int bridge_size;
    
    int ship_passengers[MAX_PASSENGERS];
    int ship_count;
    
    int queue_to_bridge_time;
    int bridge_to_ship_time;
    int ship_to_bridge_time;
    int bridge_to_exit_time;
    int t1;
    int t2;
    
    long start_time_sec;
    long start_time_usec;
    
    char log_file[256];
    
    int next_board_index;
    int next_unboard_index;
    int passengers_to_unload;
};

#endif
