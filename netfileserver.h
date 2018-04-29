#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

//structs
typedef struct open_file_data{
	FILE *fp;
	int isActive;
} Open_File_Data;

//functions
void intHandler(int signum);
int get_server_ip(char * ip_str);
