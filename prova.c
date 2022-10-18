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
    
    fflush(fptr);
    fptr = fopen("srv/usr_log.txt", "r");

    int port = 4245;
    sprintf(stringa, "%d", port);
    //strcpy(buffer, "luppi\ndylan\nvero\ntalpa\n");
    fpptr = fopen("pollo.txt", "a");
        
    while(fscanf(fptr, "%s", buffer)==1){
        if(!strcmp(buffer, stringa) || !strcmp(buffer, "diego") || !strcmp(buffer, "18-10-2022|19:1:17")){
            continue;
        }
        fprintf(fpptr, "%s\n", buffer);
    }
    
    fclose(fptr);
    fclose(fpptr);

    rename("pollo.txt", "mucca.txt");
}