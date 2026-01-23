#include "ipc.h"
#include <cstdio>
#include <cstdlib>

int create_shm(size_t size) {
    int shm_id = shmget(SHM_KEY, size, IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1) {
        perror("shmget create");
        exit(1);
    }
    return shm_id;
}

int get_shm() {
    int shm_id = shmget(SHM_KEY, 0, 0600);
    if (shm_id == -1) {
        perror("shmget get");
        exit(1);
    }
    return shm_id;
}

SharedState* attach_shm(int shm_id) {
    void* ptr = shmat(shm_id, nullptr, 0);
    if (ptr == (void*)-1) {
        perror("shmat");
        exit(1);
    }
    return static_cast<SharedState*>(ptr);
}

void detach_shm(SharedState* state) {
    if (shmdt(state) == -1) {
        perror("shmdt");
    }
}

void remove_shm(int shm_id) {
    if (shmctl(shm_id, IPC_RMID, nullptr) == -1) {
        perror("shmctl remove");
    }
}

int create_sem(int nsems) {
    int sem_id = semget(SEM_KEY, nsems, IPC_CREAT | IPC_EXCL | 0600);
    if (sem_id == -1) {
        perror("semget create");
        exit(1);
    }
    return sem_id;
}

int get_sem() {
    int sem_id = semget(SEM_KEY, 0, 0600);
    if (sem_id == -1) {
        perror("semget get");
        exit(1);
    }
    return sem_id;
}

void sem_lock(int sem_id, int sem_num) {
    struct sembuf op = {(unsigned short)sem_num, -1, 0};
    while (semop(sem_id, &op, 1) == -1) {
        if (errno == EINTR) continue;
        perror("semop lock");
        exit(1);
    }
}

void sem_unlock(int sem_id, int sem_num) {
    struct sembuf op = {(unsigned short)sem_num, 1, 0};
    while (semop(sem_id, &op, 1) == -1) {
        if (errno == EINTR) continue;
        perror("semop unlock");
        exit(1);
    }
}

void sem_wait_zero(int sem_id, int sem_num) {
    struct sembuf op = {(unsigned short)sem_num, 0, 0};
    while (semop(sem_id, &op, 1) == -1) {
        if (errno == EINTR) continue;
        perror("semop wait_zero");
        exit(1);
    }
}

void sem_set(int sem_id, int sem_num, int val) {
    if (semctl(sem_id, sem_num, SETVAL, val) == -1) {
        perror("semctl setval");
        exit(1);
    }
}

int sem_get(int sem_id, int sem_num) {
    int val = semctl(sem_id, sem_num, GETVAL);
    if (val == -1) {
        perror("semctl getval");
        exit(1);
    }
    return val;
}

void remove_sem(int sem_id) {
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl remove");
    }
}

int create_msgq() {
    int msg_id = msgget(MSG_KEY, IPC_CREAT | IPC_EXCL | 0600);
    if (msg_id == -1) {
        perror("msgget create");
        exit(1);
    }
    return msg_id;
}

int get_msgq() {
    int msg_id = msgget(MSG_KEY, 0600);
    if (msg_id == -1) {
        perror("msgget get");
        exit(1);
    }
    return msg_id;
}

void send_msg(int msg_id, long type, int data) {
    MsgBuf msg;
    msg.mtype = type;
    msg.data = data;
    while (msgsnd(msg_id, &msg, sizeof(msg.data), 0) == -1) {
        if (errno == EINTR) continue;
        perror("msgsnd");
        exit(1);
    }
}

bool recv_msg(int msg_id, long type, int& data, bool wait) {
    MsgBuf msg;
    int flags = wait ? 0 : IPC_NOWAIT;
    while (msgrcv(msg_id, &msg, sizeof(msg.data), type, flags) == -1) {
        if (errno == EINTR) continue;
        if (errno == ENOMSG) return false;
        perror("msgrcv");
        exit(1);
    }
    data = msg.data;
    return true;
}

void remove_msgq(int msg_id) {
    if (msgctl(msg_id, IPC_RMID, nullptr) == -1) {
        perror("msgctl remove");
    }
}
