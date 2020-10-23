#ifndef PTHREAD_OPERATIONS_H
#define PTHREAD_OPERATIONS_H

#include <pthread.h>

#define EMPTY_STACK -1

typedef enum rw_type {L_WRITE, L_READ} rw_type;
typedef enum func_type {F_WRITE, F_READ} func_type;

typedef struct locked_node{
    int inumber;
    struct locked_node * next;
} locked_node;

typedef struct locked_stack{
    locked_node * head;
} locked_stack;


locked_stack * create_locked_stack();
void delete_locked_stack(locked_stack * stack);
int dequeue_locked_stack(locked_stack * stack);
void queue_locked_stack(locked_stack * stack, int inumber);
void unlock_locked_stack(locked_stack * stack);

void init_latches();
void latch_lock(int, rw_type rwl_type);
void latch_unlock(int);
void destroy_latches();
void command_mutex_lock();
void command_mutex_unlock();
void inode_create_lock();
void inode_create_unlock();

#endif