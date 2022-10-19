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

    FILE* fptr, *fpptr;
    char buffer[1024];
    char stringa[1024];
    char timestamp[1024];
    int i = 0;
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    sprintf(timestamp, "%d-%d-%d|%d:%d:%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,
    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    fptr = fopen("srv/usr_online.txt", "r");
    fflush(fptr);
    fpptr = fopen("pollo.txt", "a");
    fflush(fpptr);

    int port = 4245;
    //sprintf(stringa, "%d", port);
    strcpy(stringa, "diego");
        
    while(fscanf(fptr, "%s", buffer)==1){
        if(!strcmp(buffer, stringa)){
            fprintf(fpptr, "%s\n", stringa);
            fscanf(fptr, "%s", buffer);
            fprintf(fpptr, "%d\n", port);
            fscanf(fptr, "%s", buffer);
            fprintf(fpptr, "%s\n", timestamp);
            fscanf(fptr, "%s", buffer);
            fprintf(fpptr, "%d\n", 0);
            } else{
                fprintf(fpptr, "%s\n", buffer);
            }
    }
    
    fclose(fptr);
    fclose(fpptr);

}