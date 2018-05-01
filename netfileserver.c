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

#include "netfileserver.h"
#include "libnetfiles.h"

int main(){

	struct sockaddr_in info, cli;
	unsigned int len = sizeof(cli);
	memset((char*)&info,0,sizeof(info)); //zero'd out structure
	info.sin_family = AF_INET; //IPv4 connetion 
	info.sin_port = htons(PORT); //destination port in correct byte order
	info.sin_addr.s_addr = INADDR_ANY;
	
	int sock;
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	bind(sock, (struct sockaddr*)&info, sizeof(info)); //socket is bound
	
	//now listen for client connection requests
	
	listen(sock, BACKLOG);
	
	int activeThr = 1;
	
	while(1){
	
		int* sigIntHandlArg;
		*sigIntHandlArg = accept(sock, (struct sockaddr*)&cli, &len); //accept the client request
			
		if (activeThr < BACKLOG){
			
			//create new threads to handle new client requests, if necessary
			++activeThr;
			pthread_t thr;
			pthread_create(&thr, NULL, sigIntHandler, (void*) sigIntHandlArg); //create new worker thread
			//file will be locally managed (open, close, read, write) in thread
			pthread_exit(0); //terminate worker thread
			activeThr--;
		}
		else continue;
	
	
	}


	return 0;
}

void *sigIntHandler(void* cli){

	int* cliSockFD = (int*) cli;
	
	char action;
	read(*cliSockFD, &action, 1);
	int act = act - '0';
	
	switch(act){
		case 0:
			lopen(*cliSockFD);
			break;
		case 1:
			lclose(*cliSockFD);
			break;
		case 2:
			lread(*cliSockFD);
			break;
		case 3:
			lwrite(*cliSockFD);
			break;
	}
}

void lopen(int socketFD){

	char perm;
	read(socketFD, &perm, 1);
	int permissions = perm - '0'; //file open type
	
	char localfilepath[100];
	read(socketFD, localfilepath, 99);
	localfilepath[99] = '\0'; //null terminate the path name text

	int err = open(localfilepath, perm);
	
	char buffer[5];
	
	
	if (err!=-1){
		err=-err-2; //map the file descriptor to a negative value
		buffer[0] = '0';
		
		sprintf(buffer+1, "%d", err);
		write(socketFD, buffer, 5);
	}
	else{
		int no = errno;
		buffer[0] = '1';
		
		sprintf(buffer+1, "%d", errno);
		write(socketFD, buffer, 5); //send errno back to client to report
	}
}



void lclose(int socketFD){

	char fd[4];
	read(socketFD, fd, 4);
	
	int file = atoi(fd);
	int err = close(file);
	
	char buffer[4];
	if (err==-1){
		sprintf(buffer, "%d", errno);
		write(socketFD, buffer, 4); //send errno back to client to report
	}
	else{
		sprintf(buffer, "%d", err);
		itoa(err, buffer, 10);
		write(socketFD, buffer, 4); //no error
	}
}



void lread(int socketFD){

	char fd[4];
	read(socketFD, fd, 4);
	int file = atoi(fd);
	
	char byte[50];
	read(socketFD, byte, 50);
	int numBytes = atoi(byte); //number of bytes to read
	
	void* buffer[numBytes];
	int err = read(file, buffer, numBytes);
	
	char errBuff[4];
	if (err==-1){
		sprintf(errBuff, "%d", errno);
		write(socketFD, errBuff, 4); //send errno back to client to report
	}
	else{
		sprintf(errBuff, "%d", err);
		write(socketFD, errBuff, 4); //no error
		
		write(socketFD, buffer, numBytes); //send to client actual data that was read into pointer
		char byteRead[4];
		sprintf(byteRead, "%d", err);
		write(socketFD, byteRead, 4); //send to client number of bytes that were actually read by local read()
	}
}



void lwrite(int socketFD){

	char fd[4];
	read(socketFD, fd, 4);
	int file = atoi(fd);
	
	char byte[50];
	read(socketFD, byte, 50);
	int numBytes = atoi(byte); //number of bytes to read
	
	void* buffer[numBytes];
	read(socketFD, buffer, numBytes);
	
	int err = write(file, buffer, numBytes);
	
	char errBuff[4];
	if (err==-1){
		itoa(errno, errBuff, 10);
		write(socketFD, errBuff, 4); //send errno back to client to report
	}
	else{
		itoa(err, errBuff, 10);
		write(socketFD, errBuff, 4); //no error
		
		char byteWrite[4];
		itoa(err, byteWrite, 10);
		write(socketFD, byteWrite, 4); //send to client number of bytes that were actually read by local read()
	}

}


