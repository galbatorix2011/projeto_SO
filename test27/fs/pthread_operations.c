#include "pthread_operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "state.h"
#include <errno.h>

// global variables from the main.c file
extern pthread_mutex_t commands_lock;
extern pthread_cond_t canProd;
extern pthread_cond_t canCons;

/*
 * Initializes the global mutex and conds associated with operations 
 * related to the inputCommands array 
 */
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

/*
 * Destroys the global mutex and conds associated with operations 
 * related to the inputCommands array 
 */
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


/*
 * Locks the mutex related to the inputCommands array
 */
void command_lock(){
    if (pthread_mutex_lock(&commands_lock) != 0){
        fprintf(stderr, "Error: could not lock mutex\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Unlocks the mutex related to the inputCommands array
 */
void command_unlock(){
    if (pthread_mutex_unlock(&commands_lock) != 0){
        fprintf(stderr, "Error: could not unlock mutex\n");
        exit(EXIT_FAILURE);
    }
}


/*
* Aplies the type of lock that the user chose to a certain inode 
* Input:
*    - inumber: inumber of the inode that will be locked
*   - rwl_type: type of lock in input (L_WRITE or L_READ)
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
* Unlocks a certain inode 
* Input:
*    - inumber: inumber of the inode that will be unlocked
*/
void latch_unlock(int inumber){
    if (pthread_rwlock_unlock(&inode_table[inumber].lock) != 0) {
        fprintf(stderr, "Error: could not unlock rwlock\n");
        exit(EXIT_FAILURE);
    }
}


/*
 * creates a stack which contains the inumbers of the locked inodes
 * Output:
 *    - stack: initialized stack
 */
locked_stack * create_locked_stack() {
    locked_stack *stack = malloc(sizeof(locked_stack));
    stack->head = NULL;
    return stack;
}

/*
 * Deletes the stack which contains the inumbers of the locked inodes
 */
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

/*
 * Removes the last node added to the stack and returns it
 * Input:
 *    - stack: stacks that contains the inumbers of the locked nodes
 * Output:
 *    - head: the nodes that corresponds to the head of the stack
 */
locked_node * pop_locked_stack(locked_stack * stack){
    if (!stack->head)
        return NULL;
    else{
        locked_node * head = stack->head;
        locked_node * new_head = stack->head->next;
        stack->head = new_head;
        return head;
    }
}

/*
 * Adds a new node to the locked stack with an inumber given
 * Input:
 *    - stack: stacks that contains the inumbers of the locked nodes
 *    - inumber: inumber that will be in the new node
 */
void push_locked_stack(locked_stack * stack, int inumber){
    locked_node * node = malloc(sizeof(locked_node));
    node->inumber = inumber;
    node->next = stack->head;
    stack->head = node;
}

/*
 * Goes through the stacks and for each node, unlocks its inode
 * Input:
 *    - stack: stacks that contains the inumbers of the locked nodes
 */
void unlock_locked_stack(locked_stack * stack){
    locked_node * node;
    while ((node = pop_locked_stack(stack)) != NULL){
        latch_unlock(node->inumber);
        free(node);
    }
}

/*
 * Searches the nodes of the stack until it finds a node with a certain inumber and if found, returns it
 * Input:
 *    - stack: stacks that contains the inumbers of the locked nodes
 *    - inumber: value of the inumber that  is to be searched
 * Output:
 *    - node: node that contains the inumber (NULL if the inumber isn't found)
 */
locked_node* is_inumber_locked(locked_stack *stack, int inumber){
    locked_node * node = stack->head;
    while (node){
        if (node->inumber == inumber)
            return node;
        node = node->next;
    }
    return NULL;
}
