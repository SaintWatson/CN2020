#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include "hw3.h"


int main(int argc, char* argv[]){
    int sockFD;
    float loss_rate;
    sockaddr_in sender, agent, receiver, tmp_addr;
    socklen_t sender_size, recv_size, tmp_size;
    char ip[3][50];
    int port[3];
    
    if(argc != 7){
        fprintf(stderr,"用法: %s <sender IP> <recv IP> <sender port> <agent port> <recv port> <loss_rate>\n", argv[0]);
        fprintf(stderr, "例如: ./agent local local 8887 8888 8889 0.3\n");
        exit(1);
    } 
    else {

        char LOCAL[] = "local";
        SETIP(ip[0], argv[1]);
        SETIP(ip[1], LOCAL);
        SETIP(ip[2], argv[2]);

        sscanf(argv[3], "%d", &port[0]);
        sscanf(argv[4], "%d", &port[1]);
        sscanf(argv[5], "%d", &port[2]);

        sscanf(argv[6], "%f", &loss_rate);
    }

    /*Configure settings in sender struct*/
    CONFIG(sender,  port[0], ip[0]);
    CONFIG(agent,   port[1], ip[1]);
    CONFIG(receiver,port[2], ip[2]);
   
    /*Create UDP socket*/
    sockFD = socket(PF_INET, SOCK_DGRAM, 0);

    /*bind socket*/
    bind(sockFD,(sockaddr *)&agent, sizeof(agent));

    /*Initialize size variable to be used later on*/
    sender_size = sizeof(sender);
    recv_size = sizeof(receiver);
    tmp_size = sizeof(tmp_addr);

    fprintf(stderr,"[SEND]: %s:%d\n" , ip[0], port[0]);
    fprintf(stderr,"[AGNT]: %s:%d\n" , ip[1], port[1]);
    fprintf(stderr,"[RECV]: %s:%d\n" , ip[2], port[2]);

    int total_data = 0;
    int drop_data = 0;
    int segment_size, index;
    char ipfrom[1000];
    char *ptr;
    int portfrom;
    segment seg_tmp;
    srand(time(NULL));

    
    while(1){
        /*Receive message from receiver and sender*/
        RECVFROM(sockFD, seg_tmp, tmp_addr, tmp_size, segment_size);
        
        if(segment_size > 0){
            inet_ntop(AF_INET, &tmp_addr.sin_addr.s_addr, ipfrom, sizeof(ipfrom));
            portfrom = ntohs(tmp_addr.sin_port);

            if(strcmp(ipfrom, ip[0]) == 0 && portfrom == port[0]) {
                /*segment from sender, not ack*/
                if(seg_tmp.head.ack) {
                    fprintf(stderr, "sender: ack segment\n" );
                    exit(1);
                }
                /*resolution*/
                if(seg_tmp.head.seqNumber == 0){
                    SENDTO(sockFD, seg_tmp, receiver);
                    continue;
                }
                total_data++;
                if(seg_tmp.head.fin == 1) {
                    fprintf(stderr,"get     fin\n");
                    SENDTO(sockFD, seg_tmp, receiver);
                    fprintf(stderr,"fwd     fin\n");
                }
                else {
                    index = seg_tmp.head.seqNumber;
                    if(rand() % 100 < 100 * loss_rate){
                        drop_data++;
                        fprintf(stderr,"drop	data	#%d,	loss rate = %.4f\n" , index, (float)drop_data/total_data);
                    } else{ 
                        fprintf(stderr,"get	data	#%d\n",index);
                        SENDTO(sockFD, seg_tmp, receiver);
                        fprintf(stderr,"fwd	data	#%d,	loss rate = %.4f\n",index,(float)drop_data/total_data);
                    }
                }
            } 
            else if(strcmp(ipfrom,ip[2]) == 0 && portfrom == port[2]) {
                /*segment from receiver, ack*/
                if(seg_tmp.head.ack == 0) {
                    fprintf(stderr, "receiver: non-ack segment\n" );
                    exit(1);
                }
                if(seg_tmp.head.fin == 1) {
                    fprintf(stderr,"get     finack\n");
                    SENDTO(sockFD, seg_tmp, sender);
                    fprintf(stderr,"fwd     finack\n");
                    break;
                } else {
                    index = seg_tmp.head.ackNumber;
                    fprintf(stderr,"get     ack	#%d\n", index);
                    SENDTO(sockFD, seg_tmp, sender);
                    fprintf(stderr,"fwd     ack	#%d\n", index);
                }
            }
        }
    }

    return 0;
}
