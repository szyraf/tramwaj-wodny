#include "common.h"
#include "ipc.h"
#include "logger.h"
#include <sys/time.h>
#include <sched.h>

SharedState* state;
int sem_id, msg_id;
bool signaled_for_ship[MAX_PASSENGERS];

long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int get_queue_size() {
    return (state->ship_location == TYNIEC) ? state->queue_tyniec_size : state->queue_wawel_size;
}

int get_queue_passenger(int idx) {
    return (state->ship_location == TYNIEC) ? state->queue_tyniec[idx] : state->queue_wawel[idx];
}

bool can_board_ship(int pid) {
    bool has_bike = state->passenger_has_bike[pid];
    if (state->ship_people >= state->ship_capacity_people) return false;
    if (has_bike && state->ship_bikes >= state->ship_capacity_bikes) return false;
    return true;
}

bool can_enter_bridge(int pid) {
    bool has_bike = state->passenger_has_bike[pid];
    int slots = has_bike ? 2 : 1;
    return (state->bridge_count + slots <= state->bridge_capacity);
}

void wait_for_ack(int expected_pid) {
    int data;
    while (true) {
        recv_msg(msg_id, MSG_ACK, data, true);
        if (data == expected_pid) return;
    }
}

void do_loading() {
    state->trip_num++;
    log_msg(state, "CAPTAIN", "=== Trip %d: LOADING at %s ===", 
            state->trip_num, location_name(state->ship_location));
    log_msg(state, "CAPTAIN", "Loading... Ship: %d/%d people, %d/%d bikes",
            state->ship_people, state->ship_capacity_people,
            state->ship_bikes, state->ship_capacity_bikes);
    
    state->phase = PHASE_LOADING;
    state->loading_done = false;
    memset(signaled_for_ship, 0, sizeof(signaled_for_ship));
    
    long start_time = get_time_ms();
    
    while (!state->loading_done) {
        sem_lock(sem_id, SEM_MUTEX);
        
        if (state->signal2) {
            log_msg(state, "CAPTAIN", "Signal2 received during loading - ending day");
            state->day_ended = true;
            state->loading_done = true;
            sem_unlock(sem_id, SEM_MUTEX);
            break;
        }
        
        if (state->signal1) {
            log_msg(state, "CAPTAIN", "Signal1 received - early departure");
            state->loading_done = true;
            sem_unlock(sem_id, SEM_MUTEX);
            break;
        }
        
        long elapsed = get_time_ms() - start_time;
        if (elapsed >= state->t1) {
            log_msg(state, "CAPTAIN", "Loading time T1 expired");
            state->loading_done = true;
            sem_unlock(sem_id, SEM_MUTEX);
            break;
        }
        
        if (state->ship_people >= state->ship_capacity_people) {
            log_msg(state, "CAPTAIN", "Ship is full (%d/%d people)!", 
                    state->ship_people, state->ship_capacity_people);
            state->loading_done = true;
            sem_unlock(sem_id, SEM_MUTEX);
            break;
        }
        
        for (int i = 0; i < state->bridge_size; i++) {
            int pid = state->bridge_queue[i];
            if (state->passenger_state[pid] == STATE_BRIDGE && 
                !signaled_for_ship[pid] && can_board_ship(pid)) {
                signaled_for_ship[pid] = true;
                sem_unlock(sem_id, SEM_PASSENGER_BASE + pid);
            }
        }
        
        int next_queue = -1;
        int queue_size = get_queue_size();
        for (int i = 0; i < queue_size; i++) {
            int pid = get_queue_passenger(i);
            if (state->passenger_state[pid] == STATE_QUEUE &&
                state->passenger_location[pid] == state->ship_location &&
                can_enter_bridge(pid)) {
                next_queue = pid;
                break;
            }
        }
        
        if (next_queue >= 0) {
            sem_unlock(sem_id, SEM_MUTEX);
            sem_unlock(sem_id, SEM_PASSENGER_BASE + next_queue);
            wait_for_ack(next_queue);
        } else {
            bool any_waiting = false;
            for (int i = 0; i < queue_size; i++) {
                int pid = get_queue_passenger(i);
                if (state->passenger_state[pid] == STATE_QUEUE &&
                    state->passenger_location[pid] == state->ship_location) {
                    any_waiting = true;
                    break;
                }
            }
            if (!any_waiting && state->bridge_size == 0) {
                log_msg(state, "CAPTAIN", "No more passengers at %s", 
                        location_name(state->ship_location));
                state->loading_done = true;
            }
            sem_unlock(sem_id, SEM_MUTEX);
            sched_yield();
        }
    }
    
    state->signal1 = false;
    log_msg(state, "CAPTAIN", "Loading complete: %d people, %d bikes on board",
            state->ship_people, state->ship_bikes);
}

void do_bridge_clear() {
    if (state->bridge_size == 0) return;
    
    log_msg(state, "CAPTAIN", "Clearing bridge (%d people still on bridge)...", state->bridge_size);
    state->phase = PHASE_BRIDGE_CLEAR;
    
    while (state->bridge_size > 0) {
        sem_lock(sem_id, SEM_MUTEX);
        
        if (state->bridge_size > 0) {
            int pid = state->bridge_queue[state->bridge_size - 1];
            sem_unlock(sem_id, SEM_MUTEX);
            sem_unlock(sem_id, SEM_PASSENGER_BASE + pid);
            wait_for_ack(pid);
        } else {
            sem_unlock(sem_id, SEM_MUTEX);
        }
    }
    
    log_msg(state, "CAPTAIN", "Bridge cleared!");
}

void do_sailing() {
    Location from = state->ship_location;
    Location to = (from == TYNIEC) ? WAWEL : TYNIEC;
    
    log_msg(state, "CAPTAIN", "=== SAILING from %s to %s ===", 
            location_name(from), location_name(to));
    
    state->phase = PHASE_SAILING;
    
    int elapsed = 0;
    int step = 5000;
    while (elapsed < state->t2) {
        int sleep_time = (state->t2 - elapsed < step) ? (state->t2 - elapsed) : step;
        usleep(sleep_time * 1000);
        elapsed += sleep_time;
        log_msg(state, "CAPTAIN", "Sailing... %d/%d ms", elapsed, state->t2);
        
        sem_lock(sem_id, SEM_MUTEX);
        if (state->signal2) {
            log_msg(state, "CAPTAIN", "Signal2 received during sailing - will end after arrival");
            state->day_ended = true;
        }
        sem_unlock(sem_id, SEM_MUTEX);
    }
    
    state->ship_location = to;
    log_msg(state, "CAPTAIN", "Arrived at %s!", location_name(to));
}

void do_unloading() {
    log_msg(state, "CAPTAIN", "=== UNLOADING at %s (%d passengers) ===", 
            location_name(state->ship_location), state->ship_count);
    
    state->phase = PHASE_UNLOADING;
    bool signaled_for_exit[MAX_PASSENGERS] = {false};
    
    while (state->ship_count > 0 || state->bridge_size > 0) {
        sem_lock(sem_id, SEM_MUTEX);
        
        for (int i = 0; i < state->bridge_size; i++) {
            int pid = state->bridge_queue[i];
            if (state->passenger_state[pid] == STATE_BRIDGE && !signaled_for_exit[pid]) {
                signaled_for_exit[pid] = true;
                sem_unlock(sem_id, SEM_PASSENGER_BASE + pid);
            }
        }
        
        if (state->ship_count > 0) {
            int pid = state->ship_passengers[0];
            bool has_bike = state->passenger_has_bike[pid];
            int slots = has_bike ? 2 : 1;
            
            if (state->bridge_count + slots <= state->bridge_capacity) {
                sem_unlock(sem_id, SEM_MUTEX);
                sem_unlock(sem_id, SEM_PASSENGER_BASE + pid);
                wait_for_ack(pid);
                continue;
            }
        }
        
        sem_unlock(sem_id, SEM_MUTEX);
        sched_yield();
    }
    
    log_msg(state, "CAPTAIN", "Unloading complete!");
}

int main() {
    int shm_id = get_shm();
    sem_id = get_sem();
    msg_id = get_msgq();
    state = attach_shm(shm_id);
    
    sem_lock(sem_id, SEM_CAPTAIN_READY);
    
    while (state->trip_num < state->max_trips && !state->day_ended) {
        do_loading();
        
        if (state->day_ended) {
            do_bridge_clear();
            if (state->ship_count > 0) {
                state->phase = PHASE_UNLOADING;
                do_unloading();
            }
            break;
        }
        
        do_bridge_clear();
        
        if (state->ship_count == 0) {
            log_msg(state, "CAPTAIN", "No passengers on board - skipping trip");
            continue;
        }
        
        do_sailing();
        do_unloading();
    }
    
    log_msg(state, "CAPTAIN", "=== END OF DAY ===");
    state->day_ended = true;
    state->phase = PHASE_END;
    
    sem_lock(sem_id, SEM_MUTEX);
    for (int i = 0; i < state->passenger_count; i++) {
        sem_unlock(sem_id, SEM_PASSENGER_BASE + i);
    }
    sem_unlock(sem_id, SEM_MUTEX);
    
    detach_shm(state);
    return 0;
}
