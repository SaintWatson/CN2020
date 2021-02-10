#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h> 
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <time.h>
#include "hw3.h"
#include "opencv2/opencv.hpp"
using namespace cv;

void arg_init(sockaddr_in *sender, sockaddr_in *agent,VideoCapture *cap, int argc, char *argv[]);
void sock_init(int *sockFD, sockaddr_in sender);
void frame_init(int *width, int *height, Mat *frame, VideoCapture cap);

int main(int argc, char* argv[]){

    if(argc != 5)
        ERR_EXIT("Usage: ./sender <agent IP> <agent port> <sender port> <file name>\n");
    
    VideoCapture cap;
    sockaddr_in sender, agent;
    arg_init(&sender, &agent, &cap, argc, argv);


    int sockFD;
    sock_init(&sockFD, sender);

    int width, height;
    Mat frame;
    frame_init(&width, &height, &frame, cap);

    segment tmp;

    // send video resolution
    sprintf(tmp.data, "%d %d", height, width);
    tmp.head = {0,0,0,0,0,0};
    SENDTO(sockFD, tmp, agent);



    // control blocks
    FCB *HEAD = new FCB;
    FCB *CUR  = HEAD;
    FCB *TAIL = HEAD;
    FCB *TEMP = NULL;
    HEAD->next = NULL;
    HEAD->full = false;
    HEAD->seg.head = {0,1,0,0,0,0};
    RESET(HEAD->seg.data);

    // control arguments
    int winSize = 1, threshold = 16, ret_value = 0;
    int load_id = 1, ack_id = 0, ack_target = 1, send_id = 0;
    bool hasAck = true, end = false, waiting = false;
    socklen_t ret_len = sizeof(sockaddr_in);
    long start = clock(), now = clock();

    while(true){

        //load
        cap >> frame;
        if(frame.empty()){  //put fin
            if(TAIL->seg.head.fin == 0){
                TAIL->seg.head = {(int)strlen(TAIL->seg.data), load_id, 0,1,0,0};
                TAIL->next = NULL;
                TAIL->full = true;
            }
        }
        else{   // put data
            int frameSize = strlen((char*) frame.data);
            char *buffer = new char[frameSize] ;
            memcpy(buffer, frame.data, frameSize);
            char *ptr = buffer;


            while(frameSize > 0){
                int last_len = MAX_LEN - TAIL->seg.head.length;
                int put_len = (frameSize > last_len) ? last_len : frameSize;
                memcpy(TAIL->seg.data + TAIL->seg.head.length, ptr, put_len);

                ptr += put_len;
                TAIL->seg.head.length += put_len;
                last_len -= put_len;
                frameSize -= put_len;


                if(last_len == 0){
                    TAIL->next = new FCB;
                    TAIL->full = true;
                    TAIL = TAIL->next;

                    TAIL->next = NULL;
                    TAIL->full = false;
                    TAIL->seg.head = {0,++load_id,0,0,0,0};
                    RESET(TAIL->seg.data);
                }
            }

            delete[] buffer;
        }

        //send
        if(hasAck && CUR!=NULL && CUR->full){
            SENDTO(sockFD, CUR->seg, agent);
            if(CUR->seg.head.fin){
                fprintf(stderr,"send    fin\n" );
            }
            else if(CUR->seg.head.seqNumber == send_id+1){ // send
                fprintf(stderr,"send    data    #%d,    winSize = %d\n" , CUR->seg.head.seqNumber, winSize);
                send_id++;
            }
            else{
                fprintf(stderr,"rsend   data    #%d,    winSize = %d\n" , CUR->seg.head.seqNumber, winSize);
            }

            if(CUR->seg.head.seqNumber == ack_target)            
                hasAck = false;

            CUR = CUR->next;

            if(!waiting){
                waiting = true;
                start = clock();
            }
        }

        //wait
        if(ack_id != ack_target){
            RECVFROM(sockFD, tmp, agent, ret_len, ret_value);
            if(ret_value > 0){
                if(tmp.head.fin == 1){
                    fprintf(stderr,"recv    finack\n" );
                    break;
                }
                fprintf(stderr,"recv    ack     #%d\n" , tmp.head.ackNumber);
                start = clock();

                if(tmp.head.ackNumber == ack_id+1)
                    ack_id++;

                if(ack_id == ack_target){
                    winSize = (winSize<threshold) ? winSize*2 : winSize+1;
                    ack_target += winSize;
                    hasAck = true;
                    waiting = false;
                    // fprintf(stderr,YELLOW"===== LEVEL UP: w = %d =====\n" , winSize);
                }

                if(HEAD->seg.head.seqNumber == ack_id){
                    TEMP = HEAD;
                    HEAD = HEAD->next;
                    delete TEMP;
                }

            }
        }
    
        // time out
        now = clock();
        if(now-start > CLOCKS_PER_SEC){
            winSize = 1;
            threshold = (threshold/2 > 1) ? threshold/2 : 1;
            CUR = HEAD;
            ack_target = CUR->seg.head.seqNumber;
            fprintf(stderr,"time    out,            threshold = %d\n", threshold);
            start = clock();
            waiting = false;
            hasAck = true;
        }
    }


    return 0;
}

void arg_init(sockaddr_in *sender, sockaddr_in *agent,VideoCapture *cap, int argc, char *argv[]){

    // argv[1]
    char agent_addr[IP_LEN];
    SETIP(agent_addr, argv[1]);

    // argv[2]
    int agent_port = atoi(argv[2]);
    if(agent_port < 0)
        ERR_EXIT("Illegal agent port\n");
    
    // argv[3]
    int sender_port = atoi(argv[3]);
    if(sender_port < 0)
        ERR_EXIT("Illegal sender port\n");

    // argv[4]
    *cap = VideoCapture(argv[4]);
    if(cap == NULL)
        ERR_EXIT("The video doesn't exist\n");

    // configure agent
    memset(agent, 0, sizeof(sockaddr_in));
    agent->sin_family = AF_INET;
    agent->sin_port = htons(agent_port);
    agent->sin_addr.s_addr = inet_addr(agent_addr);

    // configure sender
    memset(sender, 0, sizeof(sockaddr_in));
    sender->sin_family = AF_INET;
    sender->sin_port = htons(sender_port);
    sender->sin_addr.s_addr = INADDR_ANY;

    fprintf(stderr,"[SEND]: " "%s:%d\n" , "127.0.0.1", sender_port);
    fprintf(stderr,"[AGNT]: " "%s:%d\n" , agent_addr, agent_port);


    return;
}
void sock_init(int *sockFD, sockaddr_in sender){

    // Build an UDP connection
    if((*sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("Build socket failed\n");

    // Bind the sender info with the sockFD
    if(bind(*sockFD, (sockaddr*)&sender, sizeof(sender)) < 0)
        ERR_EXIT("Bind socket failed\n");
    
    // Set sockFD nonblocking
    fcntl(*sockFD, F_SETFL, fcntl(*sockFD, F_GETFL, 0) | O_NONBLOCK);

    return;
}
void frame_init(int *width, int *height, Mat *frame, VideoCapture cap){

    *width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    *height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
    *frame = Mat::zeros(*height, *width, CV_8UC3);
    
    if(!frame->isContinuous())
        *frame = frame->clone();

    return;
}
