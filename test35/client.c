#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>

#define INDIM 30
#define OUTDIM 512

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

int main(int argvs, char **argv) {
    int sockfd;
    socklen_t servlen, clilen;
    struct sockaddr_un serv_addr, client_addr;
    char buffer[1024];

    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);

    unlink(argv[1]); 

    clilen = setSockAddrUn(argv[1], &serv_addr);

    bind(sockfd, (struct sockaddr *) & client_addr, clilen);

    servlen = setSockAddrUn(argv[2], &serv_addr);

    sendto(sockfd, argv[3], strlen(argv[3])+1, 0, (struct sockaddr *) &serv_addr, servlen);
    recvfrom(sockfd, buffer, sizeof(buffer), 0, 0, 0);
    printf("Recebeu resposta do servidor: %s\n", buffer);
    close(sockfd);

    unlink(argv[1]);
  
    exit(EXIT_SUCCESS);

}

