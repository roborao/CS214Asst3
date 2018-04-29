#include "netfileserver.h"
#include "libnetfiles.h"

int bufferInfo[4] = 0;

int netserverint(char * hostname){

	// struct for socket address
    	struct addrinfo info, *sv, *p;
    	char s[INET6_ADDRSTRLEN]; 
    	int fileDesc, flag;
    	
    	//memset to default zero value
    	memset((char*)&info,0,sizeof(info));
    	
    	//IP, port types
    	info.ai_family = AF_UNSPEC;           // IPv4 or IPv6
    	info.ai_socktype = SOCK_STREAM;       // Sets as TCP
    	
    	// Automatically initialize the address information from host
    	if ((flag = getaddrinfo(hostname, SERV_TCP_PORT_STR, &info, &sv)) != 0) {
        	fprintf(stderr, "Client: %s\n", gai_strerror(flag));
        	return -1;
    	}
    	
    	 // Loop through server information for the appropriate address information to start a socket
   	p = sv; 
   	while(p != NULL){

		fileDesc = socket(p->ai_family, p->ai_socktype, p->ai_protocol); //open socket
		int svconnect = connect(fileDesc, p->ai_addr, p->ai_addrlen); //connect to server
		
       		
       		if (fileDesc< 0) fprintf(stderr, "Client"); //socket could not be opened
         
       		else if (svconnect < 0) close(fileDesc); //close socjet
      
		else break; //successfully opened the socket and connected
		
       		p = p->ai_next;
   	}
   	
   	// Get IP address from socket address
   	inet_ntop(p->ai_family, getaddr((struct sockaddr *)p->ai_addr), s, sizeof(s));
   	
   	// clean info
   	freeaddrinfo(sv);
    	
    	return fileDesc;
}	

void * getaddr(struct sockaddr *addr){
	
	if (addr->sa_family != AF_INET){
		return &(((struct sockaddr_in6 *)addr)->sin6_addr);
	}
	
	else{
		return &(((struct sockaddr_in*)addr)->sin_addr);
	}
}


void writeCommand(int s_fd, int type, int flag, int size, int status) {
   	
   	// Write in packet
   	bufferInfo[0] = htonl(type);
   	bufferInfo[1] = htonl(flag);
   	bufferInfo[2] = htonl(size);
   	bufferInfo[3] = htonl(status);
   	writen(s_fd, (char *)&bufferInfo, 16);
}
    	
void readCommandServer(int fileDesc, Command_packet * packet) {
		
   	// Read bytes
   	readn(fileDesc, (char *)&bufferInfo, 16);
  	packet->type = ntohl(bufferInfo[0]);
   	packet->flag = ntohl(bufferInfo[1]);
   	packet->size = ntohl(bufferInfo[2]);
  	packet->status = ntohl(bufferInfo[3]);
}

void * readCommand(int fileDesc) {

   	// Allocate memory for command packet struct
   	Command_packet * packet = (Command_packet *)malloc(sizeof(Command_packet));
   	
   	// Read bytes
  	readn(fileDesc, (char *)(&bufferInfo[0]), 16);
  	packet->type = ntohl(bufferInfo[0]);
  	packet->flag = ntohl(bufferInfo[1]);
   	packet->size = ntohl(bufferInfo[2]);
   	packet->status = ntohl(bufferInfo[3]);
  
   	return (void *)packet;
}


int readn(int fd, char * ptr, int nbytes) {
  
   	// Declare and initialize counters
   	int nleft, nread;
   	nleft = nbytes;
   	
   	// Loop through reading bytes until EOF is found
   	while (nleft > 0) {
       		nread = read(fd, ptr, nleft);

		if (nread < 0) {           // Error
           		return(nread);
       		} else if (nread == 0) {   // EOF
       	    		break;
       		}
      		nleft-=nread;
       		ptr+=nread;
   	}
   	
	// Return the number of bytes successfully read
   	return (nbytes-nleft);
}

int writen(int fd, char* ptr, int nbytes){

	//Declare and intialize counters
	int nleft, nwritten;
	nleft = nbytes;
	
	//Loop through and writing bytes until all bytes are written
	while (nleft > 0){
		nwritten = write(fd, ptr, nleft);
		
		if (nwritten <1){	//Error
			return (nwritten);
		}
		
		nleft-=nwritten;
		ptr+=nwritten;
	}
	
	//Return the number of bytes successfully written
	return (nbytes-nleft);
}

int netopen(const char* pathname, int fileDesc, int flags){

	//Send command to server
	writeCommand(fileDesc, 1, flags, strlen(pathname), 0);
	
	//Write filename to socket
	errno = 0;
	writen(fileDesc, (char*) pathname, strlen(pathname));
	if (errno != 0){
		fprintf(stderr, "Client");
		errno = 0;
	}

	//Recieve response from server
	Command_packet * cPack = (Command_packet *)readCommand(fileDesc);
	
	//Get file descriptor index and free command packet
	errno = cPack->flag;
	int fd = cPack->status;
	free(cPack);
	
	if (fd < 0){
		fprintf(stderr, "Client");
	}
	
	//Return file descriptor index recieved from server
	return fd;
}

int netclose(int fileDesc, int fd) {

   	// Send command to server
   	writeCommand(fileDesc, 2, 0, 0, fd);

   	// Receive response from server
   	Command_packet * cPack = (Command_packet *)readCommand(fileDesc);

   	// Get status and free command packet
   	errno = 0;
   	errno = cPack->flag;
   	int stat = cPack->status;
   	free(cPack);

   	if (stat < 0) {
       		fprintf(stderr, "Client");
   	}

   	// Return status received from server
   	return stat;
}

ssize_t netread(int fileDesc, int fd, void * buf, size_t nbyte) {

   	// Send command to server
   	writeCommand(fileDesc, 3, 0, nbyte, fd);

   	// Read character into buffer
   	errno = 0;
   	readn(fileDesc, (char *)buf, nbyte);
   	if (errno != 0) {
       		fprintf(stderr, "Client");
       		errno = 0;
   	}

   	// Receive response from server
   	Command_packet * cPack = (Command_packet *)readCommand(fileDesc);

   	// Get status and free command packet
   	int size = cPack->size;
   	errno = cPack->flag;
   	free(cPack);

   	// Check if the number of bytes read is correct
   	if (size != nbyte) {
       		fprintf(stderr, "Client");
   	}

   	// Return size of read from server
   	return size;
}

ssize_t netwrite(int fileDesc, int fd, const void * buf, size_t nbyte) {

	//Send command to server
	writeCommand(fileDesc, 4, 0, nbyte, fd);
	
	//Send buffer to be written to server
	errno = 0;
	writen(fileDesc, (char*)buf, nbyte);
	if (errno!=0){
		fprintf(stderr, "Client");
		errno = 0;
	}
	
	//Receive response from server
	Command_packet *cPack = (Command_packet *)readCommand(fileDesc);
	
	//get status and free command packet
	int size = cPack->size;
	errno = cPack->flag;
	free(cPack);
	
	//check if number of bytes written is correct
	if (size!=nbyte){
		fprintf(stderr, "Client");
	}
	
	//Return status received from server
	return size;
}














