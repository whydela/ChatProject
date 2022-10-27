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

bool check_word(FILE* ptr, char stringa[1024]){

    printf("Checkiamo la parola %s\n", stringa);

    char buffer[1024];

    while(fscanf(ptr, "%s", buffer)==1){
        printf("Trovo %s\n", buffer);
        if(strstr(buffer, stringa)){
            return true;
        }
    }

    return false;
}

int main(){

    FILE* fp, *fptr;
    char buffer[1024];
    char carattere;
    char stringa[1024];
    char timestamp[1024];
    int i = 0;
    int ch = 0;
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    sprintf(timestamp, "%d-%d-%d|%d:%d:%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,
    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    fp = fopen("diego/chat/lore.txt", "r");
    fptr = fopen("diego/chat/lore1.txt", "a+");
    fflush(fp);
    fflush(fptr);
    while(fscanf(fp, "%s", buffer)==1){
        //printf("Trovo %s\n", buffer);
        fflush(fptr);
        if(!strcmp(buffer, "*")){
            fprintf(fptr, "**\n");
        } else if(!strcmp(buffer, "**")){
            fprintf(fptr, "**\n");
        }
        else{
            fprintf(fptr, "%s ", buffer);
        }
        fflush(fptr);
    }
    fclose(fp);
    fclose(fptr);

    remove("diego/chat/lore.txt");
    rename("diego/chat/lore1.txt", "diego/chat/lore.txt");


    printf("%d\n", i);
}