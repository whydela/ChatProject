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

    FILE* fpptr, *fptr;
    char carattere;
    char scorre[1024];
    char* users;
    char timestamp[1024];
    strcpy(timestamp, "diego\nlore\n");
    users = timestamp;
    char lista[1024];
    char stringa[1024];
    char buffer[1024];
    int i = 0;
    int j = 0;
    int ch = 0;
    bool timestamped = true;
    bool dev_usred = false;
    int lines;
    time_t rawtime;
    struct tm * timeinfo;
    bool find = false;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    //sprintf(timestamp, "%d-%d-%d|%d:%d:%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,
    //timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);    

 
    // Opening file in reading mode
 
    // Printing what is written in file
    // character by character using loop.
    memset(lista, 0, sizeof(lista));
    memset(buffer, 0, sizeof(*buffer));

    fptr = fopen("srv/usr_online.txt", "r");

    while(fgets(scorre, 1024, fptr) != NULL){
        if(!(i%3)){     // Stiamo visionando un username
            //printf("online_user: %s", scorre);
            ch = 0;
            users = timestamp;
            while(sscanf(users, "%s", stringa)==1){
                ch = strlen(stringa)+1;
                users += ch;
                strcat(stringa, "\n");
                //printf("chat_user: %s", stringa);
                if(!strcmp(stringa, scorre)){
                    //printf("%s e' stato trovato\n", scorre);
                    find = true;
                    break;
                }
            }
            if(!find){
                strcat(buffer, "-> ");
                strcat(buffer, scorre);
            } 
            find = false;
        }
        i++;
    }

    printf("Lista degli utenti online:\n%s", buffer);

    // fclose(fptr);

    //remove("lore/chat/diego.txt");
    //rename("lore/chat/diego1.txt", "lore/chat/diego.txt");

    return 0;
 
}