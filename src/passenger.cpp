#include "common.h"
#include "ipc.h"
#include "logger.h"
#include <cstdlib>

SharedState* state;
int sem_id, msg_id;
int my_id;
bool has_bike;

void remove_from_queue() {
    int* queue;
    int* size;
    
    if (state->passenger_location[my_id] == TYNIEC) {
        queue = state->queue_tyniec;
        size = &state->queue_tyniec_size;
    } else {
        queue = state->queue_wawel;
        size = &state->queue_wawel_size;
    }
    
    for (int i = 0; i < *size; i++) {
        if (queue[i] == my_id) {
            for (int j = i; j < *size - 1; j++)
                queue[j] = queue[j + 1];
            (*size)--;
            break;
        }
    }
}

void add_to_queue_front() {
    int* queue;
    int* size;
    
    if (state->passenger_location[my_id] == TYNIEC) {
        queue = state->queue_tyniec;
        size = &state->queue_tyniec_size;
    } else {
        queue = state->queue_wawel;
        size = &state->queue_wawel_size;
    }
    
    for (int i = *size; i > 0; i--)
        queue[i] = queue[i - 1];
    queue[0] = my_id;
    (*size)++;
}

void add_to_bridge() {
    state->bridge_queue[state->bridge_size++] = my_id;
    int slots = has_bike ? 2 : 1;
    state->bridge_count += slots;
}

void remove_from_bridge() {
    for (int i = 0; i < state->bridge_size; i++) {
        if (state->bridge_queue[i] == my_id) {
            for (int j = i; j < state->bridge_size - 1; j++)
                state->bridge_queue[j] = state->bridge_queue[j + 1];
            state->bridge_size--;
            break;
        }
    }
    int slots = has_bike ? 2 : 1;
    state->bridge_count -= slots;
}

void add_to_ship() {
    state->ship_passengers[state->ship_count++] = my_id;
    state->ship_people++;
    if (has_bike) state->ship_bikes++;
}

void remove_from_ship() {
    for (int i = 0; i < state->ship_count; i++) {
        if (state->ship_passengers[i] == my_id) {
            for (int j = i; j < state->ship_count - 1; j++)
                state->ship_passengers[j] = state->ship_passengers[j + 1];
            state->ship_count--;
            break;
        }
    }
    state->ship_people--;
    if (has_bike) state->ship_bikes--;
}

char name_buf[16];
const char* my_name() {
    snprintf(name_buf, sizeof(name_buf), "P%d%s", my_id, has_bike ? "B" : "");
    return name_buf;
}

int main(int argc, char* argv[]) {
    if (argc != 2) return 1;
    
    my_id = atoi(argv[1]);
    
    int shm_id = get_shm();
    sem_id = get_sem();
    msg_id = get_msgq();
    state = attach_shm(shm_id);
    
    has_bike = state->passenger_has_bike[my_id];
    
    while (true) {
        sem_lock(sem_id, SEM_PASSENGER_BASE + my_id);
        
        sem_lock(sem_id, SEM_MUTEX);
        
        if (state->phase == PHASE_END) {
            sem_unlock(sem_id, SEM_MUTEX);
            break;
        }
        
        if (state->passenger_state[my_id] == STATE_EXITED) {
            sem_unlock(sem_id, SEM_MUTEX);
            break;
        }
        
        Phase phase = state->phase;
        int my_state = state->passenger_state[my_id];
        
        if (phase == PHASE_LOADING && my_state == STATE_QUEUE) {
            sem_unlock(sem_id, SEM_MUTEX);
            usleep(state->queue_to_bridge_time * 1000);
            sem_lock(sem_id, SEM_MUTEX);
            
            remove_from_queue();
            add_to_bridge();
            state->passenger_state[my_id] = STATE_BRIDGE;
            log_msg(state, my_name(), "Entered bridge");
            send_msg(msg_id, MSG_ACK, my_id);
            
            sem_unlock(sem_id, SEM_MUTEX);
            continue;
        }
        
        if (phase == PHASE_LOADING && my_state == STATE_BRIDGE) {
            sem_unlock(sem_id, SEM_MUTEX);
            usleep(state->bridge_to_ship_time * 1000);
            sem_lock(sem_id, SEM_MUTEX);
            
            if (state->phase != PHASE_LOADING || 
                state->ship_people >= state->ship_capacity_people ||
                (has_bike && state->ship_bikes >= state->ship_capacity_bikes)) {
                sem_unlock(sem_id, SEM_MUTEX);
                continue;
            }
            
            remove_from_bridge();
            add_to_ship();
            state->passenger_state[my_id] = STATE_SHIP;
            log_msg(state, my_name(), "Entered ship");
            send_msg(msg_id, MSG_ACK, my_id);
            
            sem_unlock(sem_id, SEM_MUTEX);
            continue;
        }
        
        if (phase == PHASE_BRIDGE_CLEAR && my_state == STATE_BRIDGE) {
            sem_unlock(sem_id, SEM_MUTEX);
            usleep(state->queue_to_bridge_time * 1000);
            sem_lock(sem_id, SEM_MUTEX);
            
            remove_from_bridge();
            add_to_queue_front();
            state->passenger_state[my_id] = STATE_QUEUE;
            log_msg(state, my_name(), "Left bridge (returned to queue)");
            send_msg(msg_id, MSG_ACK, my_id);
            
            sem_unlock(sem_id, SEM_MUTEX);
            continue;
        }
        
        if (phase == PHASE_UNLOADING && my_state == STATE_SHIP) {
            sem_unlock(sem_id, SEM_MUTEX);
            usleep(state->ship_to_bridge_time * 1000);
            sem_lock(sem_id, SEM_MUTEX);
            
            remove_from_ship();
            add_to_bridge();
            state->passenger_state[my_id] = STATE_BRIDGE;
            log_msg(state, my_name(), "Disembarked to bridge");
            send_msg(msg_id, MSG_ACK, my_id);
            
            sem_unlock(sem_id, SEM_MUTEX);
            continue;
        }
        
        if (phase == PHASE_UNLOADING && my_state == STATE_BRIDGE) {
            sem_unlock(sem_id, SEM_MUTEX);
            usleep(state->bridge_to_exit_time * 1000);
            sem_lock(sem_id, SEM_MUTEX);
            
            remove_from_bridge();
            state->passenger_state[my_id] = STATE_EXITED;
            log_msg(state, my_name(), "Left bridge");
            send_msg(msg_id, MSG_ACK, my_id);
            
            sem_unlock(sem_id, SEM_MUTEX);
            break;
        }
        
        sem_unlock(sem_id, SEM_MUTEX);
    }
    
    detach_shm(state);
    return 0;
}
