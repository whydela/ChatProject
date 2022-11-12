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
    //char buffer[1024];
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

    char buffer[] = {"lore 4246\ndylan 4247\nspera 4248\ndiego 4245\ncipo 4249"};
    char* estrai = buffer;
    char scorri[1024];
    memset(scorri, 0, sizeof(scorri));
    //struct users chat_user
    while(sscanf(estrai, "%s", scorri)==1){
        if(!scorri){
            printf("via");
        }
        if(strcmp(scorri, "dylan")){
            printf("USR: %s\n", scorri);
            //strcpy(chat_user[num_users].usr, estrai);
            ch = strlen(scorri)+1;
            estrai += ch;
            sscanf(estrai, "%s", scorri);
            printf("PORT: %s\n", scorri);
            ch = strlen(scorri)+1;
            estrai += ch;
        } else{
            ch = strlen(scorri)+1;
            estrai += ch;
            sscanf(estrai,"%s", scorri);
            ch = strlen(scorri)+1;
            estrai += ch;
        }
    }
    
    

    printf("Lista degli utenti online:\n%s\n", buffer);

    // fclose(fptr);

    //remove("lore/chat/diego.txt");
    //rename("lore/chat/diego1.txt", "lore/chat/diego.txt");

    return 0;
 
}