#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include "pthread.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
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
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(){
    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), stdin)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        if (line[0] == 'X')
            break;
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
}

void * applyComand(void * arg){
    pthread_mutex_lock(&lock);
    const char* command = removeCommand();
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

void applyCommands(){
    pthread_t pid[15];
    int i = 0;
    int j;
    while (numberCommands > 0){
        pthread_mutex_lock(&lock);
        numberCommands--;
        pthread_mutex_unlock(&lock);
        pthread_create(&pid[i++], NULL, applyComand, NULL);
    }
    for (j = 0; j < i; j++)
        pthread_join(pid[j], NULL);
}

int main(int argc, char* argv[]) {
    /* init filesystem */
    init_fs();

    /* process input and print tree */
    processInput();
    applyCommands();
    print_tecnicofs_tree(stdout);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}
