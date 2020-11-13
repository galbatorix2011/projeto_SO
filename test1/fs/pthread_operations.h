#ifndef PTHREAD_OPERATIONS_H
#define PTHREAD_OPERATIONS_H

typedef enum rw_type {L_WRITE, L_READ} rw_type;
typedef enum type_lock {L_NONE, L_MUTEX, L_RW} type_lock;

void init_latch(type_lock lock_type);
void latch_lock(type_lock lock_type, rw_type rwl_type);
void latch_unlock(type_lock lock_type);
void destroy_latch(type_lock lock_type);

#endif