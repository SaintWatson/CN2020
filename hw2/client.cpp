#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include "common.h"
    
char msg[STD_STRSIZE];

int init_client(int* svrFD, char* info);
int handle_request(int svrFD);
int handle_response(int svrFD);
int compile_cmd(char *cmd, char *arg);
void play_routine(char* filename);

int main(int argc, char *argv[]){
    if(argc != 2)
        ERR_EXIT("Error: wrong number of arguments\n");

    int svrFD;
    if(init_client(&svrFD, argv[1]) < 0)
        ERR_EXIT("Error: intialization failed\n");

    // welcome message
    recv(svrFD, msg, sizeof(msg), 0); 
    printf("%s", msg);

    while(1){
        printf("$ ");

        if(handle_request(svrFD) < 0)
            continue;

        if(handle_response(svrFD) < 0)
           ERR_EXIT("Error: recv failed\n");
    }

    return 0;
}

//============== Function Definition ====================//
int init_client(int* srvFD, char* info){
    *srvFD = socket(AF_INET, SOCK_STREAM, 0);

    // addr configuration
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));

    addr.sin_family = PF_INET;
    char *buf = strtok(info, ":");
    if( (addr.sin_addr.s_addr = inet_addr(buf) ) < 0)
        return -1;
    buf = strtok(NULL, ":");
    if(buf == NULL)
        return -1;
    if( (addr.sin_port = htons(atoi(buf))) < 0)
        return -1;

    
    // connect    
    if(connect(*srvFD, (sockaddr*)&addr, sizeof(addr)) < 0)
        return -1;

    // check the directory of client
    if(opendir("./cln")==NULL)
        mkdir("cln", 0755);
    chdir("./cln");
    
    return 0;
}
int handle_request(int svrFD){
    int ins;
    char *arg = new char;

    RESET(msg);
    fgets(msg, sizeof(msg), stdin);

    if(msg[0] == 10) // input nothing, just press <ENTER>.
        return REQ_ERR;
    if((ins = compile_cmd(msg, arg)) < 0)
        return REQ_ERR;

    int len = strlen(arg);
    int file_size;
    // send request
    switch (ins){

        case CMD_LS:    
            sprintf(msg, "%d:0:0", CMD_LS);         
            break;

        case CMD_GET:   
            sprintf(msg, "%d:0:%s", CMD_GET, arg); 
            break;

        case CMD_PUT:   
            file_size = flength(arg);
            sprintf(msg, "%d:%d:%s", CMD_PUT, file_size, arg);
            break;

        case CMD_PLAY:  
            if(len<4 || strcmp(arg+(len-4), ".mpg") != 0){
                printf("The '%s' is not a mpg file.\n", arg);
                return REQ_ERR;
            }
            sprintf(msg, "%d:0:%s", CMD_GET, arg); 
            break;

        case CMD_EXIT:  
            sprintf(msg, "%d:0:0", CMD_EXIT);        
            break;

        case CMD_CNF:   
            printf("Command not found.\n"); 
            return REQ_ERR;

        case CMD_HELP:  
            printf("\t$ ls\t\t\tlist all the files.\n");
            printf("\t$ put  [filename]\tupload the file.\n");
            printf("\t$ get  [filename]\tdownload the file.\n");
            printf("\t$ play [videoname]\tplay the video.\n");
            printf("\t$ exit\t\t\tlogout from the server\n");
            return REQ_NORECV;
    }
    
    if(send(svrFD, msg, strlen(msg), 0) < 0)
        ERR_EXIT("Error: send failed\n");

    // additional service
    switch (ins){
        case CMD_PUT:
            send_file(svrFD, arg, file_size);
            return REQ_NORECV;
        break;
        
        case CMD_GET:
            recv(svrFD, msg, sizeof(msg), 0);
            file_size = atoi(msg);
            if(file_size < 0)
                fprintf(stderr, "The '%s' doesn't exist.\n", arg);
            else
                recv_file(svrFD, arg, file_size);
            return REQ_NORECV;
        break;

        case CMD_PLAY:
            recv(svrFD, msg, sizeof(msg), 0);
            file_size = atoi(msg);
            if(file_size < 0)
                fprintf(stderr, "The '%s' doesn't exist.\n", arg);
            else
                recv_file(svrFD, arg, file_size);
            play_routine(arg);
            return REQ_NORECV;
        break;

        default:
            break;
    }

    delete arg;
    return REQ_CORRECT;
}
int handle_response(int svrFD){
    RESET(msg);
    if(recv(svrFD, msg, sizeof(msg), 0) < 0)
        return -1;

    if(msg[0] == '\a')
        ERR_EXIT("logout\n");
     
    printf("%s", msg); 
    return 0; 
}
int compile_cmd(char *cmd, char *arg){
    cmd = strtok(cmd, " \n");
    char *tmp = strtok(NULL, " \n");
    if(tmp != NULL)
        strcpy(arg, tmp); // Because we don't want to change the address of arg.
    else
        strcpy(arg, "");

    if(strtok(NULL, " \n") != NULL){ // There are still args.
        printf("Command format error\n");
        return -1;
    }

    if(strcmp(cmd, "ls") == 0){
        if(strcmp(arg, "-a") == 0){
            char* buf = new char[STD_STRSIZE];
            listdir(buf);
            printf("[LOCAL]\n%s\n[REMOTE]\n", buf);
            delete buf;
        }
        else if(strlen(arg) != 0){ // ls with argument
            printf("Command format error\n");
            return -1;
        }
        return CMD_LS;
    }
    if(strcmp(cmd, "help") == 0){
        if(strlen(arg) != 0){ // ls with argument
            printf("Command format error\n");
            return -1;
        }
        return CMD_HELP;
    }
    if(strcmp(cmd, "exit") == 0){
        if(strlen(arg) != 0){ // ls with argument
            printf("Command format error\n");
            return -1;
        }
        return CMD_EXIT;
    } 
    if(strcmp(cmd, "get") == 0) return CMD_GET;
    if(strcmp(cmd, "put") == 0){
        if(checkdir(arg) < 0){
            printf("The '%s' doesn't exist.\n",arg);
            return -1;
        }
        return CMD_PUT;
    }
    if (strcmp(cmd, "play") == 0) return CMD_PLAY;
    return CMD_CNF;
}
void play_routine(char* filename){
    int status;
    int pid = fork();
    if(pid == 0){
        execlp("../player","../player", filename);
    }
    else{
        wait(&status);
    }
    return;
}


