#include "pthread_operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t command_lock;
pthread_mutex_t mutex_lock;
pthread_rwlock_t rw_lock;

/*
* Inicializes the latches with type of lock in input and the mutex that will lock
*   variables related with inputComands
* Input:
*   -lock_type: type of lock in input (mutex, rw_lock or none)
*/
void init_latches(type_lock lock_type){
    if (lock_type == L_MUTEX){
        if (pthread_mutex_init(&mutex_lock, NULL) != 0){
            fprintf(stderr, "Error: could not initialize mutex\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (lock_type == L_RW){
        if (pthread_rwlock_init(&rw_lock, NULL) != 0){
            fprintf(stderr, "Error: could not initialize rwlock\n");
            exit(EXIT_FAILURE);
        }
    }
    if (pthread_mutex_init(&command_lock, NULL) != 0){
        fprintf(stderr, "Error: could not initialize mutex\n");
        exit(EXIT_FAILURE);
    }
}

/*
* Aplies the type of lock that the user chose 
* Input:
*   -lock_type: type of lock in input (mutex, rw_lock or none)
*/
void latch_lock(type_lock lock_type, rw_type rwl_type){
    if (lock_type == L_MUTEX){
        if (pthread_mutex_lock(&mutex_lock) != 0){
            fprintf(stderr, "Error: could not lock mutex\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (lock_type == L_RW) {
        if (rwl_type == L_WRITE){
            if (pthread_rwlock_wrlock(&rw_lock) != 0) {
                fprintf(stderr, "Error: could not lock rwlock\n");
                exit(EXIT_FAILURE);
            }
        }
        else{
            if (pthread_rwlock_rdlock(&rw_lock) != 0){
                fprintf(stderr, "Error: could not lock rwlock\n");
                exit(EXIT_FAILURE); 
            }
        }
    }
}
/*
* Unlocks the type of lock that the user chose 
* Input:
*   -lock_type: type of lock in input (mutex, rw_lock or none)
*/
void latch_unlock(type_lock lock_type){
    if (lock_type == L_MUTEX){
        if (pthread_mutex_unlock(&mutex_lock) != 0) {
            fprintf(stderr, "Error: could not unlock mutex\n");
            exit(EXIT_FAILURE);    
        }
    }
    else if (lock_type == L_RW){
        if (pthread_rwlock_unlock(&rw_lock) != 0) {
            fprintf(stderr, "Error: could not unlock rwlock\n");
            exit(EXIT_FAILURE);
        }
    }
}
/*
* Free memory releted to locks
* Input:
*   -lock_type: type of lock in input (mutex, rw_lock or none)
*/
void destroy_latches(type_lock lock_type){
    if (lock_type == L_MUTEX){
        if (pthread_mutex_destroy(&mutex_lock) != 0){
            fprintf(stderr, "Error: could not destroy mutex\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (lock_type == L_RW){
        if (pthread_rwlock_destroy(&rw_lock) != 0) {
            fprintf(stderr, "Error: could not destroy rwlock\n");
            exit(EXIT_FAILURE);
        }
    }
    if (pthread_mutex_destroy(&command_lock) != 0){
        fprintf(stderr, "Error: could not destroy mutex\n");
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
* Unlocksls variables related with inputComands
*/
void command_mutex_unlock(){
    if (pthread_mutex_unlock(&command_lock) != 0){
        fprintf(stderr, "Error: could not unlock mutex\n");
        exit(EXIT_FAILURE);
    }
}