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
#include <dirent.h>

char buffer[1024];

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

char* filetobuffer(FILE* fptr){

    char scorre[1024];
    bool timestamp = true;
    memset(buffer, 0, sizeof(*buffer));         // Pulizia della variabile globale
    while(fscanf(fptr, "%s", scorre)==1){
        if(timestamp){
            strcat(scorre, "\t");
            timestamp = false;
        }
        if(!strcmp(scorre, "*")){
            strcat(buffer, "***\n");
            timestamp = true;
            continue;
        }
        else{
            strcat(scorre, " ");
        }
        strcat(buffer, scorre);
    }

    return buffer;

}

int main(){

    FILE* fp, *fptr;
    char carattere;
    char scorre[1024];
    char timestamp[1024];
    int i = 1;
    int j = 0;
    int ch = 0;
    bool timestamped = true;
    bool dev_usred = false;
    int lines;
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    //sprintf(timestamp, "%d-%d-%d|%d:%d:%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,
    //timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);    

    fptr = fopen("srv/diego/pendent/dylan.txt", "r");
    fflush(fptr);
    char* stringa = filetobuffer(fptr);
    while(sscanf(stringa, "%s", scorre)==1){
        ch = strlen(scorre)+1;
        stringa += ch;
        if(timestamped){
            printf("%s\t", scorre);
            timestamped = false;
            continue;
        }
        if(!strcmp(scorre, ":")){
            continue;
        }
        if(!strcmp(scorre, "***")){
            printf("\n");
            timestamped = true;
        } else{
            printf("%s ", scorre);
        }
    }
}