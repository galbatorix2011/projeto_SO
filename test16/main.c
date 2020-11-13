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


#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

pthread_mutex_t commands_lock;
int producePtr = 0, consumePtr = 0, count = 0, endOfFile = 0;
pthread_cond_t canProd, canCons; 

/* global variables */

struct timespec begin, end;

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

/*---------------------*/

void insertCommand(char* data) {
	command_lock();
	while(count == MAX_COMMANDS)
		pthread_cond_wait(&canProd, &commands_lock);
	strcpy(inputCommands[producePtr++], data);
	if(producePtr == MAX_COMMANDS)
		producePtr = 0;
	count++;
	pthread_cond_signal(&canCons);
	command_unlock();
}

int removeCommand(char* command) {
	while(count == 0){
		pthread_cond_wait(&canCons, &commands_lock);
		if (count == 0 && endOfFile)
			return 0;
	}	
	strcpy(command, inputCommands[consumePtr++]);
	if(consumePtr == MAX_COMMANDS)
		consumePtr = 0;
	count--;
	pthread_cond_signal(&canProd);
	return 1;
}

void errorParse() {
	fprintf(stderr, "Error: command invalid\n");
	exit(EXIT_FAILURE);
}

void* processInput(void* arg){
	char* input_file = (char*) arg;
	FILE *ptrf;
	ptrf = fopen(input_file,"r");
	if (ptrf == NULL) {
		fprintf(stderr,"Error: could not open the input file\n");
		exit(EXIT_FAILURE);
	}

	char line[MAX_INPUT_SIZE];

	/* break loop with ^Z or ^D */

	while (fgets(line, sizeof(line)/sizeof(char), ptrf)) {
		if (line[0] == 'X'){
			break;
		}

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
				insertCommand(line);
				break;
				
			case 'l':
				if(numTokens != 2)
					errorParse();
				insertCommand(line);
				break;
			case 'd':
				if(numTokens != 2)
					errorParse();
				insertCommand(line);
				break;
				
			case '#':
				break;
	
			default: { /* error */
				errorParse();
			}
		}
	}
	if (fclose(ptrf) == EOF) {
		printf("Error: could not close the output file\n");
		exit(EXIT_FAILURE);
	}
	command_lock();
	endOfFile = 1;
	pthread_cond_broadcast(&canCons);
	command_unlock();
	return NULL;
}

/* 
* Each thread will apply a series of commands until there are none left to do
*/
void * applyCommands() {
	/* 
	* The beggining of each loop must be locked so
	* the reading of numberComands can be done safely
	*/
	locked_stack * stack = create_locked_stack();
	command_lock();
	while (count > 0 || !endOfFile){
		char command[MAX_INPUT_SIZE];
		if (!removeCommand(command))
			break;
		command_unlock();

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
						printf("Create file: %s\n",name);
						create(name, T_FILE, stack);
						break;
					case 'd':
						printf("Create directory: %s\n", name);
						create(name, T_DIRECTORY, stack);
						break;
					default:
						fprintf(stderr, "Error: invalid node type\n");
						exit(EXIT_FAILURE);
				}
				break;
			case 'l':
				/* 
				* the function lookup must be locked from the outside as it is
				* used in the fucntions create and delete from operations
				* which also lock the function loopup form the outside
				*/
				searchResult = lookup(name, stack, F_READ);  
				unlock_locked_stack(stack);             
				if (searchResult >= 0)
					printf("Search: %s found\n", name);
				else
					printf("Search: %s not found\n", name);
				break;
			case 'd':
				printf("Delete: %s\n", name);
				delete (name, stack);
				break;
			default:
				/* error */
				fprintf(stderr, "Error: command to apply\n");
				exit(EXIT_FAILURE);
		}
		/* again, lock is used so the begining of the next loop can be read safely*/
		command_lock();
	}
	/* unlocked is used since it didn't get intside the while loop and so the lock is still active */
	delete_locked_stack(stack);
	command_unlock();
	return NULL;
}

/* 
 * Initialiazes the latches, the pool of threads and the time
 * and waits for the threads to finsih their tasks
 * Input:
 *  - t_pool_size: size of the thread pool to be created
 */
void threads_initializer(int t_pool_size) {
	int i;
	
	pthread_t *pid = malloc(sizeof(pthread_t) * t_pool_size);

	/* thread pool initiazed */
	for (i = 0; i < t_pool_size; i++) {
		if (pthread_create(&pid[i], 0, applyCommands, NULL) != 0) {
			fprintf(stderr, "Error: could not create thread\n");
			exit(EXIT_FAILURE);
		}
	}

	/* the timer starts right after thread pool is initiated */
	if (clock_gettime(CLOCK_REALTIME, &begin) != 0){
		fprintf(stderr, "Error: could not start clock\n");
		exit(EXIT_FAILURE);
	}

	/* waits before all threads finish */
	for (i = 0; i < t_pool_size; i++) {
		if (pthread_join(pid[i], NULL) != 0) {
			fprintf(stderr, "Error: could not join thread\n");
			exit(EXIT_FAILURE);    
		}
	}

	free(pid);
}


/* 
 * Verifies if the input is correct
 * Input:
 *  - argc: number of arguments
 *  - argv: string of arguments
 */

void verify_input(int argc, char* argv[]) {
	int t_pool_size;
	
	if (argc != 4) {
		fprintf(stderr,"Error: number of arguments invalid\n");
		exit(EXIT_FAILURE);
	}

	if (sscanf(argv[3], "%d", &t_pool_size) != 1) { 
		fprintf(stderr,"Error: third argument isn't an int\n");
		exit(EXIT_FAILURE);
	}
	else if (t_pool_size <= 0) {
		fprintf(stderr,"Error: third argument isn't a positive int\n");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char* argv[]) {
	char *input_file = argv[1];
	char *output_file = argv[2];
	int t_pool_size;
	pthread_t inputThread;

	verify_input(argc, argv);
	/* init filesystem */
	
	init_fs();

	/* process input */
	init_command_lock();
	if (pthread_create(&inputThread, NULL, processInput, input_file) != 0){
		fprintf(stderr, "Error: could not create thread k\n");
		exit(EXIT_FAILURE);
	}

	sscanf(argv[3], "%d", &t_pool_size);
	threads_initializer(t_pool_size);

	if (pthread_join(inputThread, NULL) != 0) {
		fprintf(stderr, "Error: could not join Thread\n");
		exit(EXIT_FAILURE);
	}
	destroy_command_lock();
	
	/* calculate the time elapsed */
	if (clock_gettime(CLOCK_REALTIME, &end) != 0){
		fprintf(stderr, "Error: could not end clock\n");
		exit(EXIT_FAILURE);
	}
	double time_elapsed = (end.tv_sec - begin.tv_sec) + (double)(end.tv_nsec - begin.tv_nsec) / 1000000000L;
	printf("TecnicoFS completed in %.4f seconds.\n", time_elapsed);
	
	print_tecnicofs_tree(output_file);



	/* release allocated memory */
	destroy_fs();
	 

	exit(EXIT_SUCCESS);
}
