#ifndef PTHREAD_OPERATIONS_H
#define PTHREAD_OPERATIONS_H

#include <pthread.h>

#define EMPTY_STACK -1

typedef enum rw_type {L_WRITE, L_READ} rw_type;
typedef enum func_type {F_WRITE, F_READ, F_MOV_SECOND} func_type;

typedef struct locked_node{
    int inumber;
    struct locked_node * next;
} locked_node;

typedef struct locked_stack{
    locked_node * head;
} locked_stack;


locked_stack * create_locked_stack();
void delete_locked_stack(locked_stack * stack);
locked_node * pop_locked_stack(locked_stack * stack);
void push_locked_stack(locked_stack * stack, int inumber);
void unlock_locked_stack(locked_stack * stack);
locked_node* is_inumber_locked(locked_stack *stack, int inumber);

void lock_inode(int inumber, rw_type rwl_type);
void unlock_inode(int inumber);
void init_command_lock();
void destroy_command_lock();
void command_lock();
void command_unlock();

#endif