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

    FILE* fptr, fpptr;
    char buffer[1024];
    char stringa[1024];
    
    fptr = fopen("srv/usr_online.txt", "r");
    //strcpy(buffer, "luppi\ndylan\nvero\ntalpa\n");
        
    while(fscanf(fptr, "%s", buffer)==1){
        int i = 0;
        strcat(buffer, "*");
        while(i < 3){
            fscanf(fptr, "%s", stringa);
            strcat(buffer, stringa);
            if(i!=2){
                strcat(buffer, "*");
            }
            i++;
        }
        printf("%s\n", buffer);
    }



}