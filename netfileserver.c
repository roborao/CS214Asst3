#include "netfileserver.h"
#include "libnetfiles.h"

//Structs
typedef struct thread_data {
	int * cli_fd;
	int client_id;
	Command_packet * cPtr;
} Thread_data;

//Functions
int executeClientCommands(Thread_data * td);

//Macros
#define SERV_TCP_PORT "9000"
#define BACKLOG 5
#define THREAD_MAX 100
#define LOOP_BACK_ADDR "127.0.0.1"
#define OPEN_FILES_MAX 100

//glogbals
Open_File_Data openFiles[THREAD_MAX][OPEN_FILES_MAX];
int client_id = 0;
pthread_mutex_t m_lock = PTHREAD_MUTEX_INITIALIZER;
sig_atomic_t serv_sockfd;

void intHandler(int signum){
	fcnttl(serv_sockfd, F_SETFL, O_NONBLOCK);
}

int get_server_ip(char*ip_str){
	
	//Declare interface address structss
	struct ifaddrs *addrs, *temp;
	
	//Initalize addrs struct
	if (getifaddrs(&addrs)<0){
		perror("Server");
		return -1;
	}
	temp = addrs;
	
	//Traverse interface address linked list
	while (temp){
	
		//Check if interface address is IPv4
		if (temp->ifa_addr && temp->ifa_addr->sa_family == AF_INET){
			//Fill in temp socket address
			struct sockaddr_in *t_sockaddr = (struct sockaddr_in*)temp->ifa_addr;
			
			//if temp socket address is not the LOOP_BACK_ADDR, copy into ip_str
			if (strcmp(LOOP_BACK_ADDR, inet_ntoa(t_sockaddr->sin_addr))!=0){
			strcp(ip_str, inet_ntoa(t_sockaddr->sin_addr));
			printf("%s\n", ip_str);
			break;
			}
		}
		
		//Go to next interface address struct
		temp = temp->ifa_next;
	}
	
	//free interface address linked list
	freeifaddrs(addrs);
	
	return 0;
}

int executeClientCommands(Thread_data *td){

	// Initialize socket and client ID
   	int * sockfd = td->cli_fd;
   	Command_packet * cPtr = td->cPtr;
   	
   	//Loop through incoming client commands
   	while(1){
   		//Receive command packet from client
   		bzero(cPtr, sizeof(Command_packet));
   		readCommandServer(*sockfd, cPtr);
   		
   		//Declare and initalize buffers, counters, and file descriptors
   		int i, status, nbytes;
   		int cli_id = client_id;
   		int cmd_type = cPtr->type;
   		int flag = cPtr->flag;
   		int wr_size = cPtr->size;
   		int fd_index = cPtr->status;
   		
   		//Check if file idnex is within the limit
   		if (fd_index <0 || fd_index >OPEN_FILES_MAX-1){
   			writeCommand(*sockfd, 0, 9, -1, -1);
   			break;
   		}
   		
   		//Allocate space for buffer
   		pthread_mutex_lock(&m_lock);
   		char * buf = (char*) malloc(sizeof(char)*(wr_size+1));
   		pthread_mutex_unlock(&m_lock);
   		FILE * fd;
   		
   		switch (cmd_type){
   			case 1://Open
   			
   			//Zero buffer
   			bzero(buf, (wr_size+1));
   			
   			//Read filename
   			nbytes = readn(*sockfd, buf, wr_size);
   			
   			//Error check readn
   			if (nbytes != wr_size){
   				writeCommand(*sockfd, 0, errno, 0, -1);
   				free(buf);
   				break;
   			}
   			
   			//Open file according to flag
   			switch (flag){
   				case O_RDONLY:
   					fd = fopen(buf, "r");
   					break;
   				case O_WRONLY:
   					fd = fopen(buf, "w");
   					break;
   				case O_RDWR:
   					fd = fopen(buf, "a+");
   					break;	
   				default:
   					fd = NULL;
   					break;
   			}
   			
   			//Error check file
   			if (fd == NULL){
   				writeCommand(*sockfd, 0, errno, 0, -1);
   				free(buf);
   				break;
   			}
   			
   			//Loop through array to find first open fd_index
   			for (i = 0; i<OPEN_FILES_MAX; i++){
   				if(!openFiles[cli_id][i].isActive){
   					break;
   				}
   			}
   			
   			//Initialize open file array at corresponding fd_index
   			openFiles[cli_id][i].fp = fd;
   			openFiles[cli_id][i].isActive = 1;
   			
   			//Send response to client
   			writeCommand(*sockfd, 0, 0, 0, i);
   			
   			//Free buffer
   			free(buf);
   			
   			break;
   			
   			case 2: //Close
   			
   			//Check if file is active
   			if (!openFiles[cli_id][fd_index].isActive){
   				writeCommand(*sockfd,0,9,0,-1);
   				break;
   			}
   			
   			//close active file
   			status = fclose(openFiles[cli_id][fd_index].fp);
   			
   			//set file as inactive in open file array
   			openFiles[cli_id][fd_index].isActive = 0;
   			
   			//send message to client
   			writeCommand(*sockfd,0,0,0, status);
   			break;
   			
   			case 3: //read
   			
   			//check if file is active
   			if (!openFiles[cli_id][fd_index].isActive){
   				writeCommand(*sockfd,0,9,-1,0);
   				break;
   			}
   		
   			//get file descriptor and jump to front of file
   			fd = openFiles[cli_id][fd_index].fp;
   			fseek(fd,0,SEEK_SET);
   			
   			//Zero out buffer
   			bzero(buf,wr_size+1);
   			
   			//read one byte at a time from file across the network
   			for(i=0;i<wr_size && !feof(fd); i++){
   				buf[i] = fgetc(fd);
   			}
   			
   			//Send the bytes read to the client
   			nbytes = writen(*sockfd, buf, wr_size);
   			
   			//Error check writen
   			if (nbytes!=wr_size){
   				writeCommand(*sockfd, 0, errno, -1,0);
   			}
   			
   			//Send message to client
   			writeCommand(*sockfd, 0, 0, wr_size,0);
   			
   			//free buffer
   			free(buf);
   			
   			break;
   			
   			case 4: //write
   			
   			//check if file is active
   			if (!openFiles[cli_id][fd_index].isActive) {
                   		writeCommand(*sockfd, 0, 9, -1, 0);
                   		break;
               		}
               		
               		//get file descriptor
               		fd = openFiles[cli_id][fd_index].fp;
               		
               		//Zero out buffer
               		bzero(buf,wr_size+1);
               		
               		//read the bytes to be written from the client
               		nbytes = readn(*sockfd, buf, wr_size);
               		
               		//error check readn
               		if (nbytes != wr_size){
               			writeCommand(*sockfd, 0, errno, -1, 0);
               			break;
               		}	
               		
               		//Write one byte at time from file across the network
               		for (i=0;i<wr_size;i++){
               			fputc(buf[i],fd);
               			fflush(fd);
               		}
               		
               		//Send message to client
               		writeCommand(*sockfd,0,0,wr_size,0);
               		
               		//Free buffer
               		free(buf);
               		break;
               		
               		default:
               		//tell client to close socket
               		free(buf);
               		close(*sockfd);
               		return 1;
   		}
   	}
   	
   	return 1;
}

int main(int argc, char** argv){
	
	//initialize signal handler
	signal(SIGINT, intHandler);
	
	//initialize openFiles array
	int i, j;
	for (i = 0; i<THREAD_MAX; i++){
		for (j = 0; j< OPEN_FILES_MAX;j++){
			openFiles[i][j].isActive = 0;
		}
	}
	
	//Declare and allocate memory for IP address of server
	char *ip_str = (char*) malloc(sizeof(char)*50);
	
	//initialize IP address of server
	if (get_server_ip(ip_str)<0){
		return 0;
	}
	
	//declare socket descriptors and client length
	int cli_fd;
	socklen_t cli_len;
	
	//declare address information struct
	struct addrinfo hints, cli_addr, *servinfo, *p;
	
	//fill in struct with zeroes
	bzero((char*)&hints, sizeof(hints));
	
	//Manually initialize address information struct
	hints.ai_family = AF_UNSPEC; //ipv4 or ipv6
	hints.ai_socktype = SOCK_STREAM; //sets as TCP
	
	//automatically initialize address informmation 
	if (getaddrinfo(ip_str, SERV_TCP_PORT, &hints, &servinfo)!=0){
		perror("Server");
		exit(1);
	}
	
	//Loop through server information for the appropriate address information to start a socket
	for (p=servinfo; p!=NULL; p=p->ai_next){
		//Attempt to open socket with address information
		if((serv_sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol))<0){
			continue;
		}
		
		//binds server socket to specific port to prepare for listen()
		if (bind(serv_sockfd,p->ai_addr, p->ai_addrlen)<0){
			continue;
		}
		
		//successful connection
		break;
	}
	
	//check if socket was not bound
	if (p==NULL){
		perror("Server");
	}
	
	//free server information
	freeaddrinfo(servinfo);
	
	//server waits for incoming connection requestions; serv_sockfd will be the socket to satisfy these requests
	if (listen(serv_sockfd, BACKLOG)<0){
		perror("Server");
	}
	
	//initialize thread data
	pthread_t clientThreads[THREAD_MAX];
	Thread_data * td[THREAD_MAX];
	int flag = 0;
	i = 0;
	
	//sits on accept, waiting for new clients
	while (i<THREAD_MAX){
		//initialize client socket size
		cli_len = sizeof(cli_addr);
		
		//accept client socket
		cli_fd = accept(serv_sockfd, (struct sockaddr*) &cli_addr, &cli_len);
		
		//Error check accept()
		if (cli_fd < 0){
			break;
		}
		else {
			td[i] = (Thread_data *) malloc(sizeof(Thread_data));
			td[i]->cli_fd = (int*) malloc(sizeof(int));
			*(td[i]->cli_fd) = cli_fd;
			td[i]->client_id = client_id;
			td[i]->cPtr = (Command_packet *)malloc(sizeof(Command_packet));
		}
		
		//Open thread for client and pass in client file descriptor
		if((flag == pthread_create(&clientThreads[i],NULL,(void*)executeClientCommands,(Thread_data *)td[i]))){
			continue;
		}
		else if (flag == 0){
			i++;
			client_id++;
		}
	}
	
	//join threads
	for (j=0, flag = 0; j<i; j++){
	
		//join threads
		flag = pthread_join(clientThreads[j], NULL);
		free(td[i]);
		
		//Error checking
		if (flag){
			fprintf(stderr, "Server:pthread_join() exited with status %d\n", flag);
		}
	}
	
	//free ip address of server
	free(ip_str);
	
	//close socket
	close(serv_sockfd);
	
	return 0;
}

