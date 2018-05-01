//Macros
#define PORT 54321 //hardcoded port
#define BACKLOG 5 //queue size

//functions
void lopen(int socketFD);
void lclose(int socketFD);
void lread(int socketFD);
void lwrite(int socketFD);
void* sigIntHandler(void* cli);
