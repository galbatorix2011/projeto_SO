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

  if (argc < 2) {
    printf("Argumentos esperados:\n path_server_socket string_a_enviar\n");
    return EXIT_FAILURE;
  }

  if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0) ) < 0) {
    perror("client: can't open socket");
    exit(EXIT_FAILURE);
  }

  //Ligar ao servidor
  servlen = setSockAddrUn(argv[1], &serv_addr);
  connect(sockfd, (struct sockaddr *) &serv_addr, servlen);

  //Enviar mensagem pela ligacao estabelecida
  write(sockfd, argv[2], strlen(argv[2])+1);

  //Receber mensagem que chegue pela ligacao
  read(sockfd, buffer, sizeof(buffer));


  printf("Recebeu resposta do servidor: %s\n", buffer);

  close(sockfd);

  exit(EXIT_SUCCESS);
}

	
