#include <stdio.h>
#include "thread_pool.h"
#include <stdlib.h>
#include <pthread.h>




int main(){
    thread_pool t_pool = init_thread_pool(5);
    printf("%d\n", t_pool.size);
    clean_thread_pool(&t_pool);
    return 0;
}