#ifndef PTHREAD_OPERATIONS_H
#define PTHREAD_OPERATIONS_H

#include <pthread.h>

#define EMPTY_STACK -1

typedef enum rw_type {L_WRITE, L_READ} rw_type;
typedef enum func_type {F_WRITE, F_READ, F_MOV_DIF_FIRST, F_MOV_DIF_SECOND, F_MOV_SAME_FIRST, F_MOV_SAME_SECOND } func_type;
typedef enum move_path { FIRST_MOV, SECOND_MOV} move_path;

typedef struct locked_node{
    int inumber;
    struct locked_node * next;
    move_path m_path;
} locked_node;

typedef struct locked_stack{
    locked_node * head;
} locked_stack;


locked_stack * create_locked_stack();
void delete_locked_stack(locked_stack * stack);
int dequeue_locked_stack(locked_stack * stack);
void queue_locked_stack(locked_stack * stack, int inumber);
void unlock_locked_stack(locked_stack * stack);
int is_inumber_locked(locked_stack *stack, int inumber);
int remove_inumber_from_stack(locked_stack * stack, int inumber);

void latch_lock(int inumber, rw_type rwl_type);
void latch_unlock(int inumber);
void init_command_lock();
void destroy_command_lock();
void command_lock();
void command_unlock();

#endif