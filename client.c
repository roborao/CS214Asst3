#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "libnetfiles.h"

#define LOOP_BACK_ADDR "127.0.0.1"
#define IP_SIZE 50
#define IN_FILENAME_MAX 50
#define BUFFER_MAX 50

void get_server_ip(char* ip_str){
	
	//Declare interface address structs
	struct ifaddrs *addrs, *temp;
	
	//Initialize addrs struct
	

}
