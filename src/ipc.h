#ifndef IPC_H
#define IPC_H

#include "common.h"

int create_shm(size_t size);
int get_shm();
SharedState* attach_shm(int shm_id);
void detach_shm(SharedState* state);
void remove_shm(int shm_id);

int create_sem(int nsems);
int get_sem();
void sem_lock(int sem_id, int sem_num);
void sem_unlock(int sem_id, int sem_num);
void sem_wait_zero(int sem_id, int sem_num);
void sem_set(int sem_id, int sem_num, int val);
int sem_get(int sem_id, int sem_num);
void remove_sem(int sem_id);

int create_msgq();
int get_msgq();
void send_msg(int msg_id, long type, int data);
bool recv_msg(int msg_id, long type, int& data, bool wait = true);
void remove_msgq(int msg_id);

#endif
