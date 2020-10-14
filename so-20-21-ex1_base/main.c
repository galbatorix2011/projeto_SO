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

pthread_mutex_t lock;

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
        numberCommands--;
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
    if (ptrf == NULL){
        printf("Error: could not open the input file\n");
        exit(EXIT_FAILURE);
    }

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

    if (fclose(ptrf) == EOF){
		printf("Error: could not close the output file\n");
        exit(EXIT_FAILURE);
	}
}

void * applyCommand(void *arg){


    pthread_mutex_lock(&lock);
    char* command = getCommand();
    pthread_mutex_unlock(&lock);
    
    if (command == NULL){
        printf("hi\n");
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
            //pthread_mutex_lock(&lock);
            searchResult = lookup(name);
            //pthread_mutex_unlock(&lock);
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

void applyCommands(int t_pool_size){
    pthread_mutex_init(&lock, NULL);
    thread_pool t_pool = init_thread_pool(t_pool_size);
    pthread_t pid;
    while (numberCommands > 0){
        //printf("hey\n");
        pid = get_pthread(&t_pool);
        if (pthread_create(&pid, NULL, applyCommand, NULL) != 0){
            printf("Error: could not create new thread\n");
            exit(EXIT_FAILURE);
        }
    }
    clean_thread_pool(&t_pool);
}

int verify_input(int argc, char* argv[]){
    int t_pool_size;
    if (argc != 4){
        printf("Error: number of arguments invalid\n");
        return FAIL;
    }
    if (sscanf(argv[3], "%d", &t_pool_size) != 1){ 
        printf("Error: fourth argument isn't an int\n");
        return FAIL;
    }
    else if (t_pool_size <= 0){
        printf("Error: fourth argument isn't a positive int\n");
        return FAIL;
    }
    return SUCCESS;
}

int main(int argc, char* argv[]) {
    int t_pool_size;
    if (verify_input(argc, argv) == FAIL){
        exit(EXIT_FAILURE);
    }

    char *input_file = argv[1];
    char *output_file = argv[2];
    clock_t begin;
    clock_t end;
    /* init filesystem */
    init_fs();

    /* process input and print tree */
    processInput(input_file);

    sscanf(argv[3], "%d", &t_pool_size);
    begin = clock();

    applyCommands(t_pool_size);


    print_tecnicofs_tree(output_file);

    /* release allocated memory */

    destroy_fs();
    end = clock();
    printf("TecnicoFS completed in %.4lf seconds.\n", (double)(end - begin)/CLOCKS_PER_SEC);
    exit(EXIT_SUCCESS);
}
