#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

extern char* serverName;
int sockfd;
socklen_t servlen;
struct sockaddr_un serv_addr;


void send_command(char *line){
  printf("%s\n", line);
  if (strcmp("c a f", line) == 0)
			printf("same str\n");
  if (sendto(sockfd, line, strlen(line) + 1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  }
}

int get_feedback(){
  char res[2];
  if (recvfrom(sockfd, res, sizeof(res), 0, (struct sockaddr *)&serv_addr, &servlen) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  } 
  return atoi(res);
}

int tfsCreate(char *filename, char nodeType) {
  
  send_command(line);

  return get_feedback();
}

int tfsDelete(char *path) {
  return -1;
}

int tfsMove(char *from, char *to) {
  return -1;
}

int tfsLookup(char *path) {
  return -1;
}


int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}


int tfsMount(char * sockPath) {
  socklen_t clilen;
  struct sockaddr_un client_addr;
  char *path = "/tmp/clientSocket";

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    perror("client: can't open socket");
    exit(EXIT_FAILURE);
  }

  unlink(path);
  
  clilen = setSockAddrUn (path, &client_addr);

  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    perror("client: bind error");
    exit(EXIT_FAILURE);
  }

  servlen = setSockAddrUn(sockPath, &serv_addr);
  return 0;
}

int tfsUnmount() {
  return -1;
}
