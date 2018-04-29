#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

//Structs
typedef struct command_packet{
	int type;
	int flag;
	int size;
	int status;
} Command_packet;

//Macros
#define SERV_TCP_PORT_STR "9000"

//Functions
void *getaddr(struct sockaddr *addr);
int netserverinit(char *hostname);
void writeCommand(int sockfd, int type, int flag, int size, int status);
void readCommandServer(int sockfd, Command_packet *packet);

void *readCommand(int sockfd);
int readn(int fd, char *ptr, int nbytes);
int writen(int fd, char* ptr, int nbytes);
int netopen(const char *pathname, int sockfd, int flags);
int netclose(int sockfd, int fd);
ssize_t netread(int sockfd, int fildes, void *buf, size_t nbyte);
ssize_t netwrite(int sockfd, int fildes, const void* buf, size_t nbyte);


