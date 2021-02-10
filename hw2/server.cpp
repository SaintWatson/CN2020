#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <signal.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include "common.h"

#define DEBUG

char msg[STD_STRSIZE];
Request request[MAX_CONN_FD];

//================ Global Variable Declaration ====================//
fd_set realFDs, readyFDs; // realFDs for all the fd, readyFDs for those ready to read.
int  selectFD;

//================ Function Declaration ===================//
int init_server(int* listenFD, char* info);
int handle_request(int connFD);
void disconnect(int connFD);
//================ Signal Handler ========================//
void sigpipe_handler(int signum){
    disconnect(selectFD);
    return;
}

//================ Main Function =========================//
int main(int argc, char *argv[]){

    if(argc!=2)
        ERR_EXIT("Error: Wrong number of arguments\n");

    // initialize the listen fd
    int listenFD, connFD;
    if( (init_server(&listenFD, argv[1])) < 0)
        ERR_EXIT("Error: intialization failed\n");

    // SIGPIPE generates when one side of the socket connection is broken. 
    signal(SIGPIPE, sigpipe_handler);

    // file descripters set for select()
    FD_ZERO(&realFDs);        
    FD_SET(listenFD, &realFDs);

    while(1){
        // Because select() is destructive, reset readyFDs every loop
        readyFDs = realFDs; 
        if(select(MAX_CONN_FD,&readyFDs,NULL,NULL,NULL) < 0)
            ERR_EXIT("Error: select\n");
        
        for(selectFD=0; selectFD<MAX_CONN_FD; selectFD++){
            if( FD_ISSET(selectFD, &readyFDs) ){

                if(selectFD==listenFD){ // A new connection is coming
                    sockaddr_in client_addr;
                    int client_len = sizeof(client_addr);

                    connFD = accept(listenFD, (sockaddr*)&client_addr, (socklen_t*)&client_len);
                    if(connFD < 0)
                        ERR_EXIT("Error: accept\n");
                    strcpy(request[connFD].host, inet_ntoa(client_addr.sin_addr));
                    request[connFD].fd = connFD;
                    const char *welcome_msg = "Connected. Type 'help' for instruction helps\n";
                    send(request[connFD].fd, welcome_msg, strlen(welcome_msg), 0);
                    FD_SET(connFD, &realFDs);
                }

                else{ // Handle request from client
                    if(handle_request(selectFD) < 0)
                        disconnect(selectFD);
                }
            }
        }
    }
    return 0;
}

//================ Fucntion Definition ===================//
int init_server(int* listenFD, char* info){
    *listenFD = socket(AF_INET, SOCK_STREAM, 0);

    int port;
    if( (port = atoi(info)) < 0)
        return -1;

    // addr configuration
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    if( (addr.sin_port = htons(port)) < 0)
        return -1;     

    // binding and listening
    if(bind(*listenFD, (sockaddr*)&addr, sizeof(addr)) < 0)
        return -1;
    if(listen(*listenFD, 1024) < 0)
        return -1;

    // check the directory of server
    if(opendir("./svr")==NULL)
        mkdir("svr", 0755);
    chdir("./svr");
    return 0;
}
int handle_request(int connFD){
    
    RESET(msg);

    if(recv(connFD, msg, sizeof(msg), 0) < 0)
        return -1;
    if(msg[0] == 0) // SIGINT, NULL, etc.
        return -1;

    int ins, file_size;
    char *filename = new char;
    sscanf(msg, "%d:%d:%s", &ins, &file_size, filename);

    #ifdef DEBUG
        fprintf(stderr,"Client (%d): %s\n", connFD, msg);
    #endif

    switch(ins){
        case CMD_LS:
            listdir(msg);
            send(connFD, msg, strlen(msg), 0);
        break;
        
        case CMD_GET:
            if(checkdir(filename) < 0){
                sprintf(msg, "-1");
                send(connFD, msg, sizeof(msg), 0);
            }
            else{
                file_size = flength(filename);
                sprintf(msg, "%d", file_size);
                send(connFD, msg, sizeof(msg), 0);
                send_file(connFD, filename, file_size);
            }
        break;

        case CMD_PUT:
            recv_file(connFD, filename, file_size);
        break;

        case CMD_EXIT:
            send(connFD, "\a", 1, 0);
            disconnect(connFD);
        break;
    }

    return 0;
}
void disconnect(int connFD){
    #ifdef DEBUG
        fprintf(stderr,"fd(%d): disconnected\n", connFD);
    #endif
    FD_CLR(connFD, &realFDs);
    close(connFD);
}
