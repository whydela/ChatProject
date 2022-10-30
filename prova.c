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

int count_lines(FILE* fptr){

    int ch = 0;
    int lines = 0;

    fflush(fptr);
    while(!feof(fptr)){
        ch = fgetc(fptr);
        if(ch == '\n'){
            lines++;
        }
    }
    fclose(fptr);
    return lines;

}

int main(){

    FILE* fp, *fptr;
    char buffer[1024];
    char carattere;
    char stringa[1024];
    char timestamp[1024];
    int i = 1;
    int j = 0;
    int ch = 0;
    bool timestamped = false;
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
    lines = count_lines(fptr);
    fptr = fopen("srv/diego/pendent/dylan.txt", "r");
    fflush(fptr);
    while(fscanf(fptr, "%s", buffer)==1){
        //printf("buffer = %s, j = %d, i = %d, lines = %d.\n", buffer, j, i, lines);
        if(i==lines && !timestamped){   // Se siamo nell'ultima linea
            // Estraiamo il timestamp, che è la prima stringa 
            strcpy(timestamp, buffer);
            timestamped = true;
        }

        if(++j == 2 && !dev_usred){
            strcpy(stringa, buffer);
            dev_usred = true;
        }

        if(!strcmp(buffer, "*")){   // Questa è l'ultima stringa della linea quindi passeremo a quella successiva
            i++;
        }

    }
    //  fclose(fptr);
    printf("\nMessaggi ricevuti da %s %d.\n", stringa, lines);
    printf("Timestamp del piu' recente: %s.\n\n", timestamp);

}