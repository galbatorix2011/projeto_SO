#include "pthread_operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "state.h"

pthread_mutex_t command_lock;
pthread_mutex_t create_inode_lock;


void init_latches(){
    if (pthread_mutex_init(&command_lock, NULL) != 0 || pthread_mutex_init(&create_inode_lock, NULL) != 0){
        fprintf(stderr, "Error: could not initialize lock\n");
        exit(EXIT_FAILURE);
    }
}


/*
* Aplies the type of lock that the user chose 
* Input:
*   -lock_type: type of lock in input (mutex, rw_lock or none)
*/
void latch_lock(int inumber, rw_type rwl_type){
    if (rwl_type == L_WRITE){
        if (pthread_rwlock_wrlock(&inode_table[inumber].lock) != 0) {
            fprintf(stderr, "Error: could not lock rwlock\n");
            exit(EXIT_FAILURE);
        }
    }
    else{
        if (pthread_rwlock_rdlock(&inode_table[inumber].lock) != 0){
            fprintf(stderr, "Error: could not lock rwlock\n");
            exit(EXIT_FAILURE); 
        }
    }
    
}

/*
* Unlocks the type of lock that the user chose 
* Input:
*   -lock_type: type of lock in input (mutex, rw_lock or none)
*/
void latch_unlock(int inumber){
    if (pthread_rwlock_unlock(&inode_table[inumber].lock) != 0) {
        fprintf(stderr, "Error: could not unlock rwlock\n");
        exit(EXIT_FAILURE);
    }
}

/*
* Aplies lock to variables related with inputComands
*/
void command_mutex_lock(){
    if (pthread_mutex_lock(&command_lock) != 0){
        fprintf(stderr, "Error: could not lock mutex\n");
        exit(EXIT_FAILURE);
    }
}

/*
* Unlocks variables related with inputComands
*/
void command_mutex_unlock(){
    if (pthread_mutex_unlock(&command_lock) != 0){
        fprintf(stderr, "Error: could not unlock mutex\n");
        exit(EXIT_FAILURE);
    }
}

void inode_create_lock(){
    if (pthread_mutex_lock(&create_inode_lock) != 0){
        fprintf(stderr, "Error: could not lock mutex\n");
        exit(EXIT_FAILURE);
    }
}

/*
* Unlocks variables related with inputComands
*/
void inode_create_unlock(){
    if (pthread_mutex_unlock(&create_inode_lock) != 0){
        fprintf(stderr, "Error: could not unlock mutex\n");
        exit(EXIT_FAILURE);
    }
}

void destroy_latches(){
    if (pthread_mutex_destroy(&create_inode_lock) != 0){
        fprintf(stderr, "Error: could not destroy lock2\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_destroy(&command_lock) != 0){
        fprintf(stderr, "Error: could not destroy lock1\n");
        exit(EXIT_FAILURE);
    }
}


locked_stack * create_locked_stack() {
    locked_stack *stack = malloc(sizeof(locked_stack));
    stack->head = NULL;
    return stack;
}

void delete_locked_stack(locked_stack * stack) {
    locked_node * aux;
    locked_node * head = stack->head;
    while (head) {
        aux = head;
        head = head->next;
        free(aux);
    }
    free(stack);
}

int dequeue_locked_stack(locked_stack * stack){
    if (!stack->head)
        return EMPTY_STACK;
    else{
        int inumber = stack->head->inumber;
        locked_node * new_head = stack->head->next;
        free(stack->head);
        stack->head = new_head;
        return inumber;
    }
}

void queue_locked_stack(locked_stack * stack, int inumber){
    locked_node * node = malloc(sizeof(locked_node));
    node->inumber = inumber;
    node->next = stack->head;
    stack->head = node;
}

void unlock_locked_stack(locked_stack * stack){
    int inumber;
    while ((inumber = dequeue_locked_stack(stack)) != EMPTY_STACK){
        latch_unlock(inumber);
    }
}
