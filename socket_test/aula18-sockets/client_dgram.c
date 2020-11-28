#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>


int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

int main(int argc, char **argv) {
  int sockfd;
  socklen_t servlen, clilen;
  struct sockaddr_un serv_addr, client_addr;
  char buffer[1024];

  if (argc < 3) {
    printf("Argumentos esperados:\n path_client_socket path_server_socket string_a_enviar\n");
    return EXIT_FAILURE;
  }

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    perror("client: can't open socket");
    exit(EXIT_FAILURE);
  }

  unlink(argv[1]);
  clilen = setSockAddrUn (argv[1], &client_addr);
  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    perror("client: bind error");
    exit(EXIT_FAILURE);
  }

  if (chmod(argv[1], 00222) == -1) {
    perror("client: can't change permissions of socket");
    exit(EXIT_FAILURE);
  } 
  

  servlen = setSockAddrUn(argv[2], &serv_addr);

  if (sendto(sockfd, argv[3], strlen(argv[3])+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  } 

  printf("Recebeu resposta do servidor: %s\n", buffer);

  close(sockfd);

  unlink(argv[1]);
  
  exit(EXIT_SUCCESS);
}

	
