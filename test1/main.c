#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <pthread.h>
#include <unistd.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

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

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
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

void * applyCommand(){
    /*debug*/

    pthread_mutex_lock(&lock);
	const char* command = removeCommand();
    pthread_mutex_unlock(&lock);

    if (command == NULL){
        return NULL;
    }

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

void processCommands(int t_pool_size){
    pthread_mutex_init(&lock, NULL);
    pthread_t *pid = malloc(sizeof(pthread_t) * t_pool_size);
    int j;
    int i = 0;
    int first_cicle = 1;
    while (numberCommands > 0){
        if (first_cicle && i == (t_pool_size - 1)){
            first_cicle = 0;
        }
        else if (!first_cicle){
            if(pthread_join(pid[i], NULL) != 0)
                fprintf(stderr, "Error: could not join thread\n");
        }
        if (pthread_create(&pid[i], 0, applyCommand, NULL) != 0)
            fprintf(stderr, "Error: could not create thread\n");
        i = (i == (t_pool_size - 1)) ? 0 : i + 1;
    }
    for (j = 0; j < t_pool_size; j++)
        pthread_join(pid[j], NULL);
    free(pid);
}

void verify_input(int argc, char* argv[]){
    int t_pool_size;
    if (argc != 4){
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
    processCommands(t_pool_size);
    print_tecnicofs_tree(output_file);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}