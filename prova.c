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
    
    DIR *mydir;
    struct dirent *myfile;

    mydir = opendir("diego");
    while((myfile = readdir(mydir)) != NULL) {
        if(!strcmp(myfile->d_name, "rubrica.txt")){
            printf("%s\n", myfile->d_name);
        }
    }
    closedir(mydir);
}