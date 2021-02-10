#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h> 
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include "hw3.h"
#include "opencv2/opencv.hpp"
using namespace cv;
segment buf[BUFNUM];

void arg_init(sockaddr_in *recver, sockaddr_in *agent, int argc, char *argv[]);
void sock_init(int *sockFD, sockaddr_in recver);
void frame_init(int *width, int *height, Mat *frame, VideoCapture cap);
void flush(int b_len);

int main(int argc, char* argv[]){

    if(argc != 4)
        ERR_EXIT("Usage: %s <agent IP> <agent port> <receiver port>\n");
    
    VideoCapture cap;
    sockaddr_in recver, agent;
    arg_init(&recver, &agent, argc, argv);

    int sockFD;
    sock_init(&sockFD, recver);

    int width, height;

    int wait_ack = 0, ret_value, bid = 0;
    socklen_t ret_len = sizeof(sockaddr_in);
    segment tmp;

    while (true){
        RECVFROM(sockFD, tmp, agent, ret_len, ret_value);
        if(ret_value > 0){
            if(tmp.head.seqNumber == wait_ack){
                if(wait_ack == 0){
                    sscanf(tmp.data, "%d %d", &height, &width);
                    fprintf(stderr, "%d %d\n", height, width);
                }
                else if(tmp.head.fin == 1){
                    fprintf(stderr, "recv   fin\n" );
                    tmp.head.ack = 1;
                    tmp.head.ackNumber = tmp.head.seqNumber;
                    SENDTO(sockFD, tmp, agent);
                    fprintf(stderr, "send   finack\n" );
                    flush(bid % BUFNUM);
                    exit(0);
                }
                else{
                    fprintf(stderr, "recv   data    #%d\n" , tmp.head.seqNumber);
                    memcpy(buf[bid % BUFNUM].data, tmp.data, MAX_LEN);
                    buf[bid++ % BUFNUM].head.length = tmp.head.length;

                    tmp.head.ack = 1;
                    tmp.head.ackNumber = tmp.head.seqNumber;
                    SENDTO(sockFD, tmp, agent);
                    fprintf(stderr, "send   ack     #%d\n" , tmp.head.seqNumber);

                    // if(bid % BUFNUM == 0 && bid != 0)
                    //     flush(BUFNUM);
                }
                wait_ack++ ;
            }
            else{
                fprintf(stderr, "drop   data    #%d\n" , tmp.head.seqNumber);
                tmp.head.ackNumber = wait_ack-1;
                tmp.head.ack = 1;
                SENDTO(sockFD, tmp, agent);
                fprintf(stderr, "send   ack     #%d\n" , wait_ack-1);
            }
        }
    }


    return 0;
}

void arg_init(sockaddr_in *recver, sockaddr_in *agent, int argc, char *argv[]){

    // argv[1]
    char agent_addr[IP_LEN];
    SETIP(agent_addr, argv[1]);

    // argv[2]
    int agent_port = atoi(argv[2]);
    if(agent_port < 0)
        ERR_EXIT("Illegal agent port\n");
    
    // argv[3]
    int recver_port = atoi(argv[3]);
    if(recver_port < 0)
        ERR_EXIT("Illegal recver port\n");

    // configure agent
    memset(agent, 0, sizeof(sockaddr_in));
    agent->sin_family = AF_INET;
    agent->sin_port = htons(agent_port);
    agent->sin_addr.s_addr = inet_addr(agent_addr);

    // configure recver
    memset(recver, 0, sizeof(sockaddr_in));
    recver->sin_family = AF_INET;
    recver->sin_port = htons(recver_port);
    recver->sin_addr.s_addr = INADDR_ANY;

    fprintf(stderr,"[AGNT]: " "%s:%d\n" , agent_addr, agent_port);
    fprintf(stderr,"[RECV]: " "%s:%d\n" , "127.0.0.1", recver_port);

    return;
}
void sock_init(int *sockFD, sockaddr_in recver){

    // Build an UDP connection
    if((*sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("Build socket failed\n");

    // Bind the recver info with the sockFD
    if(bind(*sockFD, (sockaddr*)&recver, sizeof(recver)) < 0)
        ERR_EXIT("Bind socket failed\n");

    return;
}
void flush(int b_len){
    static int clip_id = 0;
    char filename[MSG_LEN];
    fprintf(stderr, "flush\n" );
    sprintf(filename, "frame_%d.mpg", clip_id);
    FILE *fp = fopen(filename, "w");

    for(int i=0; i < b_len; i++){
        fwrite(buf[i].data, 1, buf[i].head.length, fp);
    }
    
    fclose(fp);
    
    int cpid = fork();
    if(cpid == 0){
        int gcpid = fork();
        if(gcpid == 0)
            execl("./player", "./player", filename);
        else
            exit(0);
    }
    else{
        waitpid(cpid, NULL, 0);
        clip_id++;
        return;
    }
}