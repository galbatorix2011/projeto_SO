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

locked_node * dequeue_locked_stack(locked_stack * stack){
    if (!stack->head)
        return NULL;
    else{
        locked_node * head = stack->head;
        locked_node * new_head = stack->head->next;
        stack->head = new_head;
        return head;
    }
}

void queue_locked_stack(locked_stack * stack, int inumber){
    locked_node * node = malloc(sizeof(locked_node));
    node->inumber = inumber;
    node->next = stack->head;
    stack->head = node;
}


void queue_locked_stack_move(locked_stack * stack, int inumber, func_type f_type){
    locked_node * node = malloc(sizeof(locked_node));
    node->inumber = inumber;
    node->locked = 1;
    node->f_type = f_type;
    node->next = stack->head;
    stack->head = node;
}

void unlock_locked_stack(locked_stack * stack){
    locked_node * node;
    while ((node = dequeue_locked_stack(stack)) != NULL){
        printf("%d removed\n-------\n",node->inumber);
        latch_unlock(node->inumber);
        free(node);
    }
}


void unlock_locked_stack_move(locked_stack * stack){
    locked_node * node = stack->head;
    locked_node * aux_node = NULL;
    while (node){
        
        if (node->f_type == F_MOV_DIF_FIRST_O || node->f_type == F_MOV_SAME_FIRST_O || 
                    node->f_type == F_MOV_DIF_SECOND_O || node->f_type == F_MOV_SAME_SECOND_O){
            if (node->locked == 2)
                return;
            latch_unlock(node->inumber);
            if (aux_node == NULL){
                stack->head = node->next;
                free(node);
                node = stack->head;
            }
            else{
                aux_node->next = node->next;
                free(node);
                node = aux_node->next;
            }
        }
        else {
        aux_node = node;
        node = node->next;
        }
    }
}

locked_node* is_inumber_locked(locked_stack *stack, int inumber){
    locked_node * node = stack->head;
    while (node){
        if (node->inumber == inumber)
            return node;
        node = node->next;
    }
    return NULL;
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

void print_stack(locked_stack * stack){
    locked_node * node = stack->head;
    while (node){
        printf("inumber --- %d\nlocked --- %d\ntype --- %d\n----------\n", node->inumber, node->locked, node->f_type);
        node = node->next;
    }
}