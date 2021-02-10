// Constant
#define MAX_LEN 4096
#define IP_LEN 32
#define MSG_LEN 64
#define BUFNUM 32

// Data types
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct{
    int length;
    int seqNumber;
    int ackNumber;
    int fin;
    int syn;
    int ack;}header;
typedef struct{
    header head;
    char data[MAX_LEN];}segment;
typedef struct FCB{
    struct FCB* next;
    bool full;
    segment  seg;
}FCB;

//Macro
#define ERR_EXIT(str) do{ fprintf(stderr, "%s", str); exit(0);} while(0)
#define RESET(a) memset(&(a), 0, sizeof(a))
#define CONFIG(name, port, ip) do{             \
    memset(&name, 0, sizeof(sockaddr_in));     \
    name.sin_family = AF_INET;                 \
    name.sin_port = htons(port);               \
    if(ip == NULL)                             \
        name.sin_addr.s_addr = INADDR_ANY;     \
    else                                       \
        name.sin_addr.s_addr = inet_addr(ip);  \
    }while(0)
#define SENDTO(sockFD, seg, dst) sendto(sockFD, &seg, sizeof(seg), MSG_CONFIRM, (sockaddr*)&(dst), sizeof(dst))
#define RECVFROM(sockFD, seg, src, ret_len, ret_value) do{ \
    memset(&(seg), 0, sizeof(seg)); \
    ret_value = recvfrom(sockFD, &seg, sizeof(seg), 0, (sockaddr*)&(src), &ret_len); \
}while(0)

#define SETIP(dst, src) do{\
    if(strcmp(src, "0.0.0.0") == 0 || strcmp(src, "local") == 0 \
         || strcmp(src, "localhost") == 0)\
        sscanf("127.0.0.1", "%s", dst); \
    else                                \
        sscanf(src, "%s", dst);         \
    }while(0)



