#include "pthread_operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "state.h"

extern pthread_mutex_t commands_lock;
extern pthread_cond_t canProd;
extern pthread_cond_t canCons;

void init_command_lock(){
    if (pthread_mutex_init(&commands_lock, NULL) != 0){
        fprintf(stderr, "Error: could not init mutex\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&canProd, NULL) != 0){
        fprintf(stderr, "Error: could not init cond\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&canCons, NULL) != 0){
        fprintf(stderr, "Error: could not init cond\n");
        exit(EXIT_FAILURE);
    }
}

void destroy_command_lock(){
    if (pthread_mutex_destroy(&commands_lock) != 0){
        fprintf(stderr, "Error: could not destroy mutex\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_destroy(&canProd) != 0){
        fprintf(stderr, "Error: could not destroy cond\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_destroy(&canCons) != 0){
        fprintf(stderr, "Error: could not destroy cond\n");
        exit(EXIT_FAILURE);
    }

}

void command_lock(){
    if (pthread_mutex_lock(&commands_lock) != 0){
        fprintf(stderr, "Error: could not lock mutex\n");
        exit(EXIT_FAILURE);
    }
}

void command_unlock(){
    if (pthread_mutex_unlock(&commands_lock) != 0){
        fprintf(stderr, "Error: could not unlock mutex\n");
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






locked_stack * create_locked_stack() {
    locked_stack *stack = malloc(sizeof(locked_stack));
    stack->head = NULL;
    return stack;
}

move_stacks * create_move_stacks(){
    move_stacks *stacks = malloc(sizeof(move_stacks));
    stacks->origin_stack = create_locked_stack();
    stacks->dest_stack = create_locked_stack();
    stacks->already_locked_stack = create_locked_stack();
    return stacks;
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

void destroy_move_stacks(move_stacks * stacks){
    delete_locked_stack(stacks->origin_stack);
    delete_locked_stack(stacks->already_locked_stack);
    delete_locked_stack(stacks->already_locked_stack);
    free(stacks);
}

void unlock_first_locked_stack(move_stacks * stacks){
    int inumber;
    while ((inumber = pop_locked_stack(stacks->origin_stack)) != EMPTY_STACK){
        if (is_inumber_locked(stacks->already_locked_stack, inumber)){
            push_locked_stack(stacks->origin_stack, inumber);
        }
        latch_unlock(inumber);
    }
}

void unlock_move_stacks(move_stacks * stacks){
    unlock_locked_stack(stacks->origin_stack);
    unlock_locked_stack(stacks->dest_stack);
}

int pop_locked_stack(locked_stack * stack){
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

void push_locked_stack(locked_stack * stack, int inumber){
    locked_node * node = malloc(sizeof(locked_node));
    node->inumber = inumber;
    node->next = stack->head;
    stack->head = node;
}

void unlock_locked_stack(locked_stack * stack){
    int inumber;
    while ((inumber = pop_locked_stack(stack)) != EMPTY_STACK){
        latch_unlock(inumber);
    }
}

int is_inumber_locked(locked_stack *stack, int inumber){
    locked_node * node = stack->head;
    while (node){
        if (node->inumber == inumber)
            return 1;
        node = node->next;
    }
    return 0;
}

int remove_inumber_from_stack(locked_stack * stack, int inumber){
    locked_node * node = stack->head;
    locked_node * aux_node = NULL;
    while (node){
        if (node->inumber == inumber){
            if (aux_node == NULL)
                stack->head = node->next;
            else{
                aux_node->next = node->next;
            }
            free(node);
            return 1;
        }
        aux_node = node;
        node = node->next;
    }
    return 0;
}
