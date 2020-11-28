#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef API_H
#define API_H

#include "../tecnicofs-api-constants.h"

int tfsCreate(char *filename, char nodeType);
int tfsDelete(char *path);
int tfsLookup(char *path);
int tfsMove(char *from, char *to);
int tfsPrintTree(char *filename);
int tfsMount(char * sockPath);
int tfsUnmount();
int setSockAddrUn(char *path, struct sockaddr_un *addr);
void send_command(char *line);
int get_feedback();

#endif /* CLIENT_H */
