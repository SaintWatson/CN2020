#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "common.h"

void listdir(char* buf){
    DIR *dir = opendir(".");
    struct dirent *ptr;
    RESET(buf);

    while((ptr=readdir(dir)) != NULL){
        if(strcmp(ptr->d_name,".") == 0 || strcmp(ptr->d_name,"..") == 0)
            continue;
        strcat(buf, ptr->d_name);
        strcat(buf, "\n");
    }
    closedir(dir);
    return;
}
int checkdir(char* target){
    DIR *dir = opendir(".");
    struct dirent *ptr;
    int flag = -1;
    while((ptr = readdir(dir)) != NULL){
        if(strcmp(target, ptr->d_name) == 0){
            flag = 0;
            break;
        }
    }
    closedir(dir);
    return flag;
}
int recv_file(int connFD, char* filename, int size){
    FILE *fp = fopen(filename, "w");
    char buf[BUF_SIZE]; 
    int numbytes;
    int o_size = size;

    while(size > 0){
		numbytes = recv(connFD, buf, sizeof(buf), 0);
        if(numbytes < 0)   
            return -1;
		numbytes = fwrite(buf, sizeof(char), numbytes, fp);
        size -= numbytes;
        fprintf(stderr, "\rReceive:%s <<< (%.2lf%%)", filename, (double)(o_size-size)*100/o_size);
	}
    fclose(fp);
    fprintf(stderr,"\rReceive %s succesfully. (%d bytes)\n", filename, o_size);
    return 0;
}
int send_file(int connFD, char* filename, int size){
    FILE *fp = fopen(filename, "r");
    char buf[BUF_SIZE];
    int numbytes;
    int o_size = size;

    while(size > 0){
		numbytes = fread(buf, sizeof(char), sizeof(buf), fp);
        if(numbytes < 0)
            return -1;
		numbytes = send(connFD, buf, numbytes, 0);
        size -= numbytes;
        fprintf(stderr, "\rSend:%s >>> (%.2lf%%)", filename, (double)(o_size-size)*100/o_size);
	}
    fclose(fp);
    fprintf(stderr,"\rSend %s succesfully. (%d bytes)\n", filename, o_size);
    return 0;
}
int flength(char* filename){
    FILE *fp = fopen(filename, "r");
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    fclose(fp);
    return len;
}