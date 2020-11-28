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
#include <errno.h>

// ---------------------------
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/uio.h>
#include <sys/stat.h>
// --------------------------

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

void errorParse() {
	fprintf(stderr, "Error: command invalid\n");
	exit(EXIT_FAILURE);
}


int receive_command(int sockfd, char* in_buffer, struct sockaddr_un *client_addr) {
	int c;
	socklen_t addrlen = sizeof(struct sockaddr_un);
	while ((c = recvfrom(sockfd, in_buffer, MAX_INPUT_SIZE, 0,
				(struct sockaddr *) client_addr, &addrlen)) <= 0);
//	printf(" command --- %s\n ", in_buffer);
	return c;
}
/* 
* Each thread will apply a series of commands until there are none left to do
*/
void * applyCommands(void *arg) {

	/* 
	 * we only manage to go to the next look if there are commands to read
	 * or if there are no commands left to read
	 */

	int sockfd = *((int*) arg);

	struct sockaddr_un client_addr;
	char in_buffer[MAX_INPUT_SIZE];
	locked_stack * stack = create_locked_stack();
	while (1){

		receive_command(sockfd, in_buffer, &client_addr);
		//if (strcmp("c a f", in_buffer) == 0)
		//	printf("same str\n");
		//printf("%s\n", in_buffer);	
		
		//printf("erro -- %d\n%d\n---------\n", bytes, errno);
		//char command[MAX_INPUT_SIZE];

		char token, type;
		char name[MAX_INPUT_SIZE], secondName[MAX_INPUT_SIZE];
		
		int numTokens = sscanf(in_buffer, "%c %s %c", &token, name, &type);
		if (token == 'm'){
			numTokens = sscanf(in_buffer, "%c %s %s", &token, name, secondName);
		}

		//printf("%c\n%s\n%c\n", token, name, type);
		if (numTokens < 2) {
			fprintf(stderr, "Error: invalid command in Queue\n");
			exit(EXIT_FAILURE);
		}

		int res = 0;
		int searchResult;
		switch (token) {
			case 'c':
				switch (type) {
					case 'f':
						printf("Create file: %s\n",name);
						res = create(name, T_FILE, stack);
						break;
					case 'd':
						printf("Create directory: %s\n", name);
						res = create(name, T_DIRECTORY, stack);
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
				res = searchResult;
				break;
			case 'd':
				printf("Delete: %s\n", name);
				res = delete (name, stack);
				break;
			case 'm':
				printf("Move: %s to %s\n", name, secondName);
				if ((res = move(name, secondName, stack)) == SUCCESS)
					printf("Move from %s to %s completed\n", name, secondName);
				break;
			case 'p':
				printf("Printing current Tree in file %s\n", name);
				res = print_tecnicofs_tree(name);
				break;
			default:
				/* error */
				fprintf(stderr, "Error: command to apply\n");
				exit(EXIT_FAILURE);
		}
		//printf("res --- %d\n", res);
		char * res_str = res == SUCCESS ? "0\0" : "1\0";
		//sendto(sockfd, res_str, sizeof(res_str), 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_un));
		/* again, lock is used so the begining of the next loop can be read safely*/
		if (sendto(sockfd, res_str, sizeof(res_str), 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_un)) < 0){
			fprintf(stderr, "Error: Could not send msg\n");
			exit(EXIT_FAILURE);
		}
	}
	/* unlocked is used since it didn't get intside the while loop and so the lock is still active */
	delete_locked_stack(stack);
	return NULL;
}

/* 
 * Initialiazes the latches, the pool of threads and the time
 * and waits for the threads to finsih their tasks
 * Input:
 *  - t_pool_size: size of the thread pool to be created
 */
void threads_initializer(int t_pool_size, int sockfd) {
	int i;
	
	pthread_t *pid = malloc(sizeof(pthread_t) * t_pool_size);
	/* thread pool initiazed */
	for (i = 0; i < t_pool_size; i++) {
		if (pthread_create(&pid[i], 0, applyCommands, &sockfd) != 0) {
			fprintf(stderr, "Error: could not create thread\n");
			exit(EXIT_FAILURE);
		}
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
	
	if (argc != 3) {
		fprintf(stderr,"Error: number of arguments invalid\n");
		exit(EXIT_FAILURE);
	}

	if (sscanf(argv[1], "%d", &t_pool_size) != 1) { 
		fprintf(stderr,"Error: third argument isn't an int\n");
		exit(EXIT_FAILURE);
	}
	else if (t_pool_size <= 0) {
		fprintf(stderr,"Error: third argument isn't a positive int\n");
		exit(EXIT_FAILURE);
	}
}

void start_countDown() {
	if (clock_gettime(CLOCK_REALTIME, &begin) != 0){
		fprintf(stderr, "Error: could not start clock\n");
		exit(EXIT_FAILURE);
	}
}

double finish_countDown(){
	if (clock_gettime(CLOCK_REALTIME, &end) != 0){
		fprintf(stderr, "Error: could not end clock\n");
		exit(EXIT_FAILURE);
	}
	return (end.tv_sec - begin.tv_sec) + (double)(end.tv_nsec - begin.tv_nsec) / 1000000000L;
}


int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

int init_server_socket(char *path){
	int sockfd;
 	struct sockaddr_un server_addr;
  	socklen_t addrlen;


	if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
    perror("server: can't open socket");
    exit(EXIT_FAILURE);
  	}
	
	unlink(path);

	addrlen = setSockAddrUn (path, &server_addr);

  	if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
    	perror("server: bind error");
    	exit(EXIT_FAILURE);
  	}

	return sockfd;
}

int main(int argc, char* argv[]) {
	int sockfd;
	double time_elapsed;
	char *output_file = argv[2];
	int t_pool_size;
	//char *socket_path;

	/*very input*/
	verify_input(argc, argv);

	//socket_path = argv[2];

	/* init filesystem */
	init_fs();

	/* process input */
	sockfd = init_server_socket(argv[2]);

	start_countDown();
	/* procces commands */
	sscanf(argv[1], "%d", &t_pool_size);
	threads_initializer(t_pool_size, sockfd);

	
	/* calculate the time elapsed */
	time_elapsed = finish_countDown();
	printf("TecnicoFS completed in %.4f seconds.\n", time_elapsed);
	
	print_tecnicofs_tree(output_file);

	/* release allocated memory */
	destroy_fs();
	
	exit(EXIT_SUCCESS);
}
