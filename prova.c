#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>

int main(){

    FILE* fptr;
    char buffer[1024];
    char stringa[1024];
    fptr = fopen("prova.txt", "r+");

    strcpy(stringa, "solo");

    while(fscanf(fptr, "%s", buffer)==1){
        printf("%s\n", buffer);
        if(!strcmp(buffer, "dylan")){
            freopen(NULL, "r+", fptr);
            fprintf(fptr, "%s", stringa);
        }
    }

    //printf("%s", stringa);



}