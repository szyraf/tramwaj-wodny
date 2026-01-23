#include "common.h"
#include "config.h"
#include "ipc.h"
#include "logger.h"
#include <sys/wait.h>
#include <iostream>
#include <vector>

std::vector<pid_t> g_children;

void cleanup_ipc() {
    int shm_id = shmget(SHM_KEY, 0, 0600);
    if (shm_id != -1) shmctl(shm_id, IPC_RMID, nullptr);
    
    int sem_id = semget(SEM_KEY, 0, 0600);
    if (sem_id != -1) semctl(sem_id, 0, IPC_RMID);
    
    int msg_id = msgget(MSG_KEY, 0600);
    if (msg_id != -1) msgctl(msg_id, IPC_RMID, nullptr);
}

void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}

void signal_handler(int sig) {
    (void)sig;
    for (pid_t p : g_children) {
        kill(p, SIGTERM);
    }
    while (wait(nullptr) > 0);
    cleanup_ipc();
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config.env>" << std::endl;
        return 1;
    }
    
    cleanup_ipc();
    
    Config cfg;
    if (!load_config(argv[1], cfg)) return 1;
    if (!validate_config(cfg)) return 1;
    
    std::cout << "=== Water Tram Simulator ===" << std::endl;
    print_config(cfg);
    
    int total_passengers = cfg.tyniec_people + cfg.tyniec_bikes + cfg.wawel_people + cfg.wawel_bikes;
    int num_sems = SEM_PASSENGER_BASE + total_passengers;
    
    int shm_id = create_shm(sizeof(SharedState));
    int sem_id = create_sem(num_sems);
    int msg_id = create_msgq();
    
    SharedState* state = attach_shm(shm_id);
    memset(state, 0, sizeof(SharedState));
    
    init_logger(state);
    
    state->phase = PHASE_INIT;
    state->ship_location = TYNIEC;
    state->trip_num = 0;
    state->max_trips = cfg.R;
    state->ship_capacity_people = cfg.N;
    state->ship_capacity_bikes = cfg.M;
    state->bridge_capacity = cfg.K;
    state->queue_to_bridge_time = cfg.queue_to_bridge_time;
    state->bridge_to_ship_time = cfg.bridge_to_ship_time;
    state->ship_to_bridge_time = cfg.ship_to_bridge_time;
    state->bridge_to_exit_time = cfg.bridge_to_exit_time;
    state->t1 = cfg.T1;
    state->t2 = cfg.T2;
    state->passenger_count = total_passengers;
    
    int pid = 0;
    for (int i = 0; i < cfg.tyniec_people; i++, pid++) {
        state->passenger_state[pid] = STATE_QUEUE;
        state->passenger_location[pid] = TYNIEC;
        state->passenger_has_bike[pid] = false;
        state->queue_tyniec[state->queue_tyniec_size++] = pid;
    }
    for (int i = 0; i < cfg.tyniec_bikes; i++, pid++) {
        state->passenger_state[pid] = STATE_QUEUE;
        state->passenger_location[pid] = TYNIEC;
        state->passenger_has_bike[pid] = true;
        state->queue_tyniec[state->queue_tyniec_size++] = pid;
    }
    for (int i = 0; i < cfg.wawel_people; i++, pid++) {
        state->passenger_state[pid] = STATE_QUEUE;
        state->passenger_location[pid] = WAWEL;
        state->passenger_has_bike[pid] = false;
        state->queue_wawel[state->queue_wawel_size++] = pid;
    }
    for (int i = 0; i < cfg.wawel_bikes; i++, pid++) {
        state->passenger_state[pid] = STATE_QUEUE;
        state->passenger_location[pid] = WAWEL;
        state->passenger_has_bike[pid] = true;
        state->queue_wawel[state->queue_wawel_size++] = pid;
    }
    
    sem_set(sem_id, SEM_MUTEX, 1);
    sem_set(sem_id, SEM_CAPTAIN_READY, 0);
    for (int i = 0; i < total_passengers; i++) {
        sem_set(sem_id, SEM_PASSENGER_BASE + i, 0);
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGCHLD, sigchld_handler);
    
    log_msg(state, "MAIN", "Created %d passengers", total_passengers);
    log_msg(state, "MAIN", "Tyniec queue: %d, Wawel queue: %d", 
            state->queue_tyniec_size, state->queue_wawel_size);
    
    pid_t captain_pid = fork();
    if (captain_pid == -1) { perror("fork captain"); cleanup_ipc(); return 1; }
    if (captain_pid == 0) {
        execl("./captain", "captain", nullptr);
        perror("execl captain");
        _exit(1);
    }
    g_children.push_back(captain_pid);
    
    pid_t dispatcher_pid = fork();
    if (dispatcher_pid == -1) { perror("fork dispatcher"); cleanup_ipc(); return 1; }
    if (dispatcher_pid == 0) {
        execl("./dispatcher", "dispatcher", nullptr);
        perror("execl dispatcher");
        _exit(1);
    }
    g_children.push_back(dispatcher_pid);
    
    for (int i = 0; i < total_passengers; i++) {
        pid_t p = fork();
        if (p == -1) { perror("fork passenger"); cleanup_ipc(); return 1; }
        if (p == 0) {
            char id_str[16];
            snprintf(id_str, sizeof(id_str), "%d", i);
            execl("./passenger", "passenger", id_str, nullptr);
            perror("execl passenger");
            _exit(1);
        }
        g_children.push_back(p);
    }
    
    log_msg(state, "MAIN", "All processes started");
    
    state->phase = PHASE_LOADING;
    sem_unlock(sem_id, SEM_CAPTAIN_READY);
    
    int status;
    while (wait(&status) > 0);
    
    std::cout << "\n[MAIN] All processes finished. Cleaning up IPC resources..." << std::endl;
    
    detach_shm(state);
    cleanup_ipc();
    
    std::cout << "[MAIN] Simulation ended successfully." << std::endl;
    return 0;
}
