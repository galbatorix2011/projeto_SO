#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <pthread.h>
#include <stdlib.h>

typedef struct thread_pool {
    int index;
    int size;
    int start;
    pthread_t * pool;
} thread_pool ;


thread_pool init_thread_pool(int size);
pthread_t get_pthread(thread_pool *t_pool);
void clean_thread_pool(thread_pool *t_pool);


#endif 