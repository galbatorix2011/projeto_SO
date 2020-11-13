#include "pthread_operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex_lock;
pthread_rwlock_t rw_lock;

void init_latch(type_lock lock_type){
    if (lock_type == L_MUTEX)
        pthread_mutex_init(&mutex_lock, NULL);
    else if (lock_type == L_RW)
        pthread_rwlock_init(&rw_lock, NULL);
}

void latch_lock(type_lock lock_type, rw_type rwl_type){
    if (lock_type == L_MUTEX){
        pthread_mutex_lock(&mutex_lock);
    }
    else if (lock_type == L_RW) {
        if (rwl_type == L_WRITE){
            pthread_rwlock_wrlock(&rw_lock);
        }
        else
            pthread_rwlock_rdlock(&rw_lock);
    }
}

void latch_unlock(type_lock lock_type){
    if (lock_type == L_MUTEX)
        pthread_mutex_unlock(&mutex_lock);
    else if (lock_type == L_RW)
        pthread_rwlock_unlock(&rw_lock);  
}

void destroy_latch(type_lock lock_type){
    if (lock_type == L_MUTEX)
        pthread_mutex_destroy(&mutex_lock);
    else if (lock_type == L_RW)
        pthread_rwlock_destroy(&rw_lock);
}