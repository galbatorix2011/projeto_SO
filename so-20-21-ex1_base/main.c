#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include "fs/thread_pool.h"
#include <unistd.h>
#include <time.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
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

char* getCommand() {
    if(numberCommands > 0){
        /*numberCommands--;*/
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/* alterar para ler de um ficheiro*/
void processInput(char * input_file){
    FILE *ptrf;
    ptrf = fopen(input_file,"r");

    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), ptrf)) {
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
    fclose(ptrf);
}

void * applyCommand(void *arg){

    pthread_mutex_lock(&mutex);
    const char* command = getCommand();
    pthread_mutex_unlock(&mutex);

    if (command == NULL)
        return NULL;

    char token, type;
    char name[MAX_INPUT_SIZE];
    int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
    if (numTokens < 2) {
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    }

    int searchResult;
    switch (token) {
        case 'c':
            switch (type) {
                case 'f':
                    printf("Create file: %s\n", name);
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
            searchResult = lookup(name);
            if (searchResult >= 0)
                printf("Search: %s found\n", name);
            else
                printf("Search: %s not found\n", name);
            break;
        case 'd':
            printf("Delete: %s\n", name);
            delete(name);
            break;
        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }
    return NULL;
}

void applyCommands(thread_pool * t_pool){
    pthread_t pid;
    while (numberCommands > 0){
        //printf("hey\n");
        pid = get_pthread(t_pool);
        numberCommands--;
        pthread_create(&pid, NULL, applyCommand, NULL);
    }
}


int main(int argc, char* argv[]) {
    char *input_file = argv[1];
    char *output_file = argv[2];
    int t_pool_size;
    clock_t begin;
    clock_t end;
    /* init filesystem */
    init_fs();

    /* process input and print tree */
    processInput(input_file);

    sscanf(argv[3], "%d", &t_pool_size);
    thread_pool t_pool = init_thread_pool(t_pool_size);
    begin = clock();

    applyCommands(&t_pool);

    print_tecnicofs_tree(output_file);

    /* release allocated memory */
    destroy_fs();
    end = clock();
    printf("TecnicoFS completed in %.4lf seconds.\n", (double)(end - begin)/CLOCKS_PER_SEC);
    exit(EXIT_SUCCESS);
}
