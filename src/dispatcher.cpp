#include "common.h"
#include "ipc.h"
#include "logger.h"
#include <iostream>
#include <termios.h>
#include <poll.h>

SharedState* state;
int sem_id;

int main() {
    int shm_id = get_shm();
    sem_id = get_sem();
    state = attach_shm(shm_id);
    
    std::cout << "\n=== Dispatcher Controls ===" << std::endl;
    std::cout << "Press '1' - Signal1: Early departure" << std::endl;
    std::cout << "Press '2' - Signal2: End day" << std::endl;
    std::cout << "===========================\n" << std::endl;
    
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    
    while (true) {
        if (state->phase == PHASE_END || state->day_ended) break;
        
        int ret = poll(&pfd, 1, 100);
        if (ret > 0 && (pfd.revents & POLLIN)) {
            char c;
            if (read(STDIN_FILENO, &c, 1) == 1) {
                sem_lock(sem_id, SEM_MUTEX);
                
                if (c == '1' && !state->signal1) {
                    state->signal1 = true;
                    log_msg(state, "DISPATCHER", "Signal1 sent - early departure");
                } else if (c == '2' && !state->signal2) {
                    state->signal2 = true;
                    state->day_ended = true;
                    log_msg(state, "DISPATCHER", "Signal2 sent - ending day");
                }
                
                sem_unlock(sem_id, SEM_MUTEX);
            }
        }
        
        if (state->phase == PHASE_END || state->day_ended) break;
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    detach_shm(state);
    return 0;
}
