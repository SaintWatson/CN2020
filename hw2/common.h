// Constant
#define STD_STRSIZE 512
#define MAX_CONN_FD 1024
#define BUF_SIZE 1024
#define BAR_SIZE 20

// Data types
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct {
    char host[STD_STRSIZE];  // client's host
    char buf[STD_STRSIZE];  // data sent by/to client
    int fd;  // fd to talk with client
} Request;


// CMD symbols
#define CMD_LS   0
#define CMD_GET  1
#define CMD_PUT  2
#define CMD_PLAY 3
#define CMD_HELP 4
#define CMD_EXIT 5
#define CMD_CNF  6

// Constant
#define REQ_CORRECT 0
#define REQ_NORECV -1
#define REQ_ERR -2

// Macros
#define RESET(a) bzero((a), sizeof(a))
#define ERR_EXIT(a) do{fprintf(stderr,"%s",a); exit(0);}while(0) 
#define CAT(str) if((str) != NULL)printf("CAT: %s(%ld)\n", (str), strlen((str)))

// function
void listdir(char* buf);
int checkdir(char* target);
int recv_file(int connFD, char* filename, int size);
int send_file(int connFD, char* filename, int size);
int flength(char *filename);