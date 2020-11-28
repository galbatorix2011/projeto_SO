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

int main(int args, char **argv) {
    int sockfd;
    struct sockaddr_un server_adrr;
    socklen_t adrrlen;
    char *path;

    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);

    path = argv[1];

    unlink(path);
    adrrlen = setSockAddrUn(path, &server_adrr); 
    
    bind(sockfd,(struct sockaddr *) &server_adrr, adrrlen);

    while (1) {
    struct sockaddr_un client_addr;
    char in_buffer[INDIM], out_buffer[OUTDIM];
    int c;

    adrrlen = sizeof(struct sockaddr_un);
    c = recvfrom(sockfd, in_buffer, sizeof(in_buffer) - 1, 0,
		 (struct sockaddr *)&client_addr, &adrrlen);
         
    if (c <= 0) continue;
    in_buffer[c]='\0';
    
    printf("Recebeu mensagem de %s\n", client_addr.sun_path);

    c = sprintf(out_buffer, "Ola' %s, que tal vai isso?", in_buffer);
    
    sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, adrrlen);

  }
    
}