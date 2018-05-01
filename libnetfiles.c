#include "netfileserver.h"
#include "libnetfiles.h"

int bufferInfo[4] = {0};

int socketFD;

int netserverint(char * hostname){

	socketFD = socket(AF_INET, SOCK_STREAM, 0); //IPv4 connection
	
	if (socketFD<0){
		fprintf(stderr, "client side: socket connection could not be established\n");
		h_errno = HOST_NOT_FOUND;
		return -1; //error
	}
	
	struct sockaddr_in info;
	memset(&info,0,sizeof(info)); //zero'd out structure
	
	info.sin_family = AF_INET; //IPv4 connetion 
	info.sin_port = htons(PORT); //destination PORT in correct byte order
	if (gethostbyname(hostname) != NULL) info.sin_addr.s_addr = *gethostbyname(hostname)->h_addr; //get and check host IP address
	else{
		fprintf(stderr, "client side: host address is invalid and could not be found\n");
		h_errno = HOST_NOT_FOUND;
		return -1; //error
	}
	
	if (connect(socketFD,(struct sockaddr*)&info, sizeof(info)) == -1){
		fprintf(stderr, "client side: could not bind to socket, connection could not be established successfully\n");
		h_errno = HOST_NOT_FOUND;
		return -1; //error
	}
	
    	return 0; //success
}	

int netopen(const char* pathname, int flags){

	//outbound talk to server to open file
	char op = '0';
	write(socketFD, &op, 1); //let server know client wants to open file

	char flag;
	ssize_t flagBytes;
	switch(flags){
		case 0: //O_RDONLY
			flag = '0';
			break;
		case 1: //O_WRONLY
			flag = '1';
			break;
		case 2: //O_RDWR
			flag = '2';
			break;
	
	}
	write(socketFD, &flag, 1);
	
	write(socketFD, pathname, strlen(pathname)); //tell server pathname of file that is to be opened

	//inbound talk from server	
	char scs[5];

	read(socketFD, scs, 5);
	
	if((scs[0]-'0')!=-1){
		int i = 0;
		char newdescrip[4];
		for (i = 0; i < 4; i++){
			newdescrip[i] = scs[i+1];
		}
		return atoi(newdescrip);
	}
	else{
		int i = 0;
		char err[4];
		for (i = 0; i<4; i++){
			err[i] = scs[i+1];
		}
		errno = atoi(err);
		fprintf(stderr, "server side: could not open file: %s\n", strerror(errno));
		return -1;
	}
	
}

int netclose(int fd) {

	fd = -fd-2; //remap

	//outbound talk to server to close file
	char cl = '1';
	write(socketFD, &cl, 1); //let server know client wants to close file
	
	char buffer[4];
	sprintf(buffer, "%d", fd);
	write(socketFD, buffer, 4); //send file descriptor
		
	//inbound talk from server 
	char success[4];
	read(socketFD, success, 4);
	if (atoi(success)==0) return 0;
	else{
		errno = atoi(success);
		fprintf(stderr, "could not close file: %s\n", strerror(errno));
		return -1;
	}
}

ssize_t netread(int fildes, void * buf, size_t nbyte) {

	fildes = -fildes-2; //remap

   	//outbound talk to server to read file
   	char rd = '2';
   	write(socketFD, &rd, 1); //let server know client wants to read file
   	
   	char buffer[4];
   	sprintf(buffer, "%d", fildes);
   	write(socketFD, buffer, 4); //send file descriptor
   	
   	char byte[50];
   	sprintf(byte, "%zd", nbyte);
   	write(socketFD, byte, 1);
   	
   	//inbound talk from server
   	char success[4];
   	read(socketFD, success, 4);
   	if (atoi(success)==0){
   		read(socketFD, buf, nbyte);
   		read(socketFD, byte, 4);
   		return atoi(byte);		
   	}
   	else{
		errno = atoi(success);
		fprintf(stderr, "could not read file: %s\n", strerror(errno));
		return -1;
   	}
   	
}

ssize_t netwrite(int fildes, const void * buf, size_t nbyte) {

	fildes = -fildes-2; //remap

	//outbound talk to server to read file
   	char wr = '3';
   	write(socketFD, &wr, 1); //let server know client wants to read file
   	
   	char buffer[4];
   	sprintf(buffer, "%d", fildes);
   	write(socketFD, buffer, 4); //send file descriptor
   	
   	char byte[50];
   	sprintf(byte, "%zd", nbyte);
   	write(socketFD, byte, 1);
   	
   	write(socketFD, buf, nbyte);
   	
   	//inbound talk from server
   	char success[4];
   	read(socketFD, success, 1);
   	if (atoi(success)==0){
   		read(socketFD, byte, 1);
   		return atoi(byte);   		
   	}
   	else{
		errno = atoi(success);
		fprintf(stderr, "could not read file: %s\n", strerror(errno));
		return -1;
   	}

}














