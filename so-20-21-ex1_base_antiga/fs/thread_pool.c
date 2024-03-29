#include "thread_pool.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

//pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

thread_pool init_thread_pool(int size){
    thread_pool t_pool;
    t_pool.index = 0;
    t_pool.size = size;
    t_pool.pool = malloc(sizeof(pthread_t) * size);
    return t_pool;
}

pthread_t get_pthread(thread_pool *t_pool){
    pthread_join(t_pool->pool[t_pool->index], NULL);
    pthread_t thread = t_pool->pool[t_pool->index];
    t_pool->index = t_pool->index = (t_pool->size - 1) ? 0 : t_pool->index++; /* cicle through the array */
    return thread;
}

void clean_thread_pool(thread_pool *t_pool){
    int i;
    for (i = 0; i < t_pool->size; i++)
        pthread_join(t_pool->pool[t_pool->index], NULL);
    free(t_pool->pool);
}

