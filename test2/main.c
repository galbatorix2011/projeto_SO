#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include "fs/pthread_operations.h"
#include <pthread.h>
#include <unistd.h>
#include <time.h>


#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

type_lock t_lock;

struct timespec begin, end;

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char *removeCommand(){
    command_mutex_lock();
    int aux_headQueue;
    if (numberCommands > 0) {
        numberCommands--;
        /* headQueue is saved in local variable aux_headQueue since it can't be altered after the unlock */
        aux_headQueue = headQueue++;
        command_mutex_unlock();
        return inputCommands[aux_headQueue];
    }
    command_mutex_unlock();
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(char * input_file){
    FILE *ptrf;
    ptrf = fopen(input_file,"r");
    if (ptrf == NULL){
        fprintf(stderr,"Error: could not open the input file\n");
        exit(EXIT_FAILURE);
    }

    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), ptrf)) {
        if (line[0] == 'X')
            break;
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
    if (fclose(ptrf) == EOF){
        printf("Error: could not close the output file\n");
        exit(EXIT_FAILURE);
    }
}

/* 
 * Each thread will apply a series of commands until there are none left to do
 */
void * applyCommands(){
    /* 
     * The beggining of each loop must be locked so
     * the reading of numberComands can be done safely
     */
    command_mutex_lock();
    while (numberCommands > 0) {
        command_mutex_unlock();
        
        const char *command = removeCommand();

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        if (command == NULL) {
            /* lock is used again so the beggining of the next loop can be read safely again */
            command_mutex_lock();
            continue;
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n",name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                /* 
                 * the function lookup must be locked from the outside as it is
                 * used in other functions such as create and delete
                 * and therefore it can't be locked from the inside
                 */
                latch_lock(t_lock, L_READ);
                searchResult = lookup(name);                
                latch_unlock(t_lock);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete (name);
                break;
            default:
                /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        /* again, lock is used again so the beggining of the next loop can be read safely again*/
        command_mutex_lock();
    }
    /* unlocked is used since it didn't get intside the while loop and therefore the lock is still active */
    command_mutex_unlock();
    return NULL;
}

/* 
 * Initialiazes the latches, the pool of threads and the timer
 * Input:
 *  - t_pool_size: size of the thread pool to be created
 */
void threads_initializer(int t_pool_size){
    int i;

    init_latches(t_lock);
    pthread_t *pid = malloc(sizeof(pthread_t) * t_pool_size);

    /*thread pool initiated*/
    for (i = 0; i < t_pool_size; i++){
        if (pthread_create(&pid[i], 0, applyCommands, NULL) != 0){
            fprintf(stderr, "Error: could not create thread\n");
            exit(EXIT_FAILURE);
        }
    }

    /* the timers starts right after thread pool is initiated */
    if (clock_gettime(CLOCK_REALTIME, &begin) != 0){
        fprintf(stderr, "Error: could not start clock\n");
        exit(EXIT_FAILURE);
    }

    /* waits before all threads finish */
    for (i = 0; i < t_pool_size; i++){
        if (pthread_join(pid[i], NULL) != 0){
            fprintf(stderr, "Error: could not join thread\n");
            exit(EXIT_FAILURE);    
        }
    }

    free(pid);
    destroy_latches(t_lock);
}


/* 
 * Verifies if the input is correct
 * Input:
 *  - argc: number of arguments
 *  - argv: string of arguments
 */

void verify_input(int argc, char* argv[]){
    int t_pool_size;
    if (argc != 5){
        fprintf(stderr,"Error: number of arguments invalid\n");
        exit(EXIT_FAILURE);
    }
    if (sscanf(argv[3], "%d", &t_pool_size) != 1){ 
       fprintf(stderr,"Error: fourth argument isn't an int\n");
        exit(EXIT_FAILURE);
    }
    else if (t_pool_size <= 0){
        fprintf(stderr,"Error: fourth argument isn't a positive int\n");
        exit(EXIT_FAILURE);
    }
    if (strcmp("mutex", argv[4]) == 0)
        t_lock = L_MUTEX;
    else if (strcmp("rwlock", argv[4]) == 0)
        t_lock = L_RW;
    else if (strcmp("nosync", argv[4]) == 0){
        if (t_pool_size != 1){
            fprintf(stderr,"Error: Lock type is nosync but there is more than one thread\n");
            exit(EXIT_FAILURE);
        }
        t_lock = L_NONE;
    }
    else{
        fprintf(stderr,"Error: wrong input for sync type\n");
        exit(EXIT_FAILURE);
    }   
}

int main(int argc, char* argv[]) {
    char *input_file = argv[1];
    char *output_file = argv[2];
    int t_pool_size;

    verify_input(argc, argv);
    /* init filesystem */
    
    init_fs();

    /* process input and print tree */
    processInput(input_file);

    sscanf(argv[3], "%d", &t_pool_size);
    threads_initializer(t_pool_size);
    print_tecnicofs_tree(output_file);

    /* release allocated memory */
    destroy_fs();

    /* calculate the time elapsed */
    if (clock_gettime(CLOCK_REALTIME, &end) != 0){
        fprintf(stderr, "Error: could not end clock\n");
        exit(EXIT_FAILURE);
    }
    double time_elapsed = (end.tv_sec - begin.tv_sec) + (double)(end.tv_nsec - begin.tv_nsec) / 1000000000L;
    
    printf("TecnicoFS completed in %.4f seconds.\n", time_elapsed);
    exit(EXIT_SUCCESS);
}







