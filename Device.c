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
#include <sys/wait.h>
#include <dirent.h>

#define CMD_CHATOFF "/CHATOFF\0"
#define CMD_CHAT "/CHAT\0"
#define CMD_GRPCHAT "/GRPCHAT\0"
#define CMD_HANGING "/HANGING\0"
#define CMD_LOG "/LOGIN\0"
#define CMD_OFF "/OFF\0"
#define CMD_PORT "/PORT\0"
#define CMD_REG "/REG\0"
#define CMD_RUBRIC "/RUBRIC\0"
#define CMD_SHOW "/SHOW\0"
#define CMD_TMS "/TIMESTAMP\0"
#define YES "/YES\0"
#define NO "/NO\0"
#define SHARE "/SHARE\0"
#define OFFLINE "/OFFLINE\0"
#define RFD "RFD\0"
#define GRPEXIT "/GRPEXIT\0"
#define EXIT "\\q\0"
#define DELETE "\\d\0"
#define ADD "\\u\0"
#define STDIN 0

char username[1024];                    // username del device
char password[1024];                    // password del device
char timestamp[1024];                   // username*timestamp*porta del device
char percorso[1024];                    // percorso: username/file.txt
char rubrica[1024];                     // rubrica
char pendent[1024];
char messaggio[1024];                   // messaggio della chat
char all_chat[1024];                    // variabile globale per la funzione "filetobuffer"
char grp_users[1024];                   // variabile contenente gli username e la loro porta che il
                                        // Device ha invitato nella chat di gruppo

int my_port;                            // porta del device
int srv_sd;                             // communication socket descriptor (Server)
int dev_sd;                             // communication socket descriptor (Device)
int listener;                           // Socket di ascolto
bool online = false;                    // se true, Device online
bool chatting = false;                  // se true, Device dentro la chat

int num_users = 0;

struct users{
    char usr[1024];
    int port;
    int sd;
};

struct users dev_users[10];

// Questa funzione si occupa della prima stampa riguardo al Device
void first_print(){
    int i;
    printf("\n");
    for(i=0; i < 190; i++){
        printf("*");
    }
    printf("\n");
    for (i=0; i < 11; i++){
        printf("\t");
    }
    printf("DEVICE AVVIATO\n");
    for(i=0; i < 190; i++){
        printf("*");
    }    
}

// Questa funzione controlla se una data 'stringa' in ingresso e' presente nel file puntato da 'ptr'
bool check_word(FILE* ptr, char stringa[1024]){

    //printf("Checkiamo la parola %s\n", stringa);

    char buffer[1024];

    while(fscanf(ptr, "%s", buffer)==1){
        //printf("Trovo %s\n", buffer);
        if(strstr(buffer, stringa)){
            return true;
        }
    }

    return false;
}

// Questa funzione si occupa della conversione di un file in buffer 
char* filetobuffer(FILE* fptr){

    char scorre[1024];
    // bool timestamp = true;
    memset(all_chat, 0, sizeof(all_chat));         // Pulizia della variabile globale
    while (fgets(scorre, 1024, fptr) != NULL) {
        strcat(all_chat, scorre);
        fflush(fptr);
    }
    
    /*
    while(fscanf(fptr, "%s", scorre)==1){
        if(timestamp){
            strcat(scorre, "\t");
            timestamp = false;
        }
        if(!strcmp(scorre, "**")){
            strcat(all_chat, "**\n");
            timestamp = true;
            continue;
        } else if(!strcmp(scorre, "***")){
            strcat(all_chat, "\n");
            timestamp = true;
            continue;
        }
        else{
            strcat(scorre, " ");
        }
        strcat(all_chat, scorre);
    }
    */

    return all_chat;

}

int ip_config(struct sockaddr_in* addr, int port){

    int sd;
    int ret;
    
    sd = socket(AF_INET, SOCK_STREAM, 0);   // Creazione socket
    memset(addr, 0, sizeof(*addr));         // Pulizia

    // Configurazione dell'indirizzo
    (*addr).sin_family = AF_INET;
    (*addr).sin_port = htons(port);
    (*addr).sin_addr.s_addr = INADDR_ANY;

    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    // Aggancio dell'indirizzo
    ret = bind(sd, (struct sockaddr*)addr, sizeof(*addr));

    if(ret < 0){
        printf("\nATTENZIONE ! Porta non corretta o gia' utilizzata da un altro device !\n\n");
        exit(-1); 
    }

    return sd;

}

int srv_config(struct sockaddr_in* srv_addr, int port){

    int sd;

    sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(srv_addr, 0, sizeof(*srv_addr)); // Pulizia 
    (*srv_addr).sin_family = AF_INET;          // Address family
    (*srv_addr).sin_port = htons(port);

    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    inet_pton(AF_INET, "127.0.0.1", &(*srv_addr).sin_addr);

    return sd;

}

int dev_connect(struct sockaddr_in* dev_addr, int port){

    int sd;

    sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(dev_addr, 0, sizeof(*dev_addr));     // Pulizia 
    (*dev_addr).sin_family = AF_INET;           // Address family
    (*dev_addr).sin_port = htons(port);

    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    inet_pton(AF_INET, "127.0.0.1", &(*dev_addr).sin_addr);

    return sd;

}

void send_srv(int sd, char* cmd){

    //printf("Invio %s\n", cmd);
    send(sd, cmd, strlen(cmd)+1, 0);

    //printf("Messaggio %s inviato\n", cmd);
    
}

int send_dev(int sd, char* cmd){
    
    int ret;
    //printf("Invio %s\n", cmd);
    ret = send(sd, cmd, strlen(cmd)+1, 0);

    return ret;
}

/*
bool ls(char* directory, char* file){

    DIR *mydir;
    struct dirent *myfile;

    mydir = opendir(directory);
    while((myfile = readdir(mydir)) != NULL) {
        if(!strcmp(myfile->d_name, file)){
            return true;
        }
    }
    closedir(mydir);
    return false;
}
*/
/*
// Cleaner che cambia il singolo asterisco con quello doppio nelle chat
void cleaner(char filename[1024]){
    
    FILE* fptr, *fpptr;
    char buffer[1024];
    char file[1024];
    char file1[1024];
    //bool timestamp = true;

    strcpy(file, filename);
    strcpy(file1, filename);

    strcat(file, ".txt");
    strcat(file1, "1.txt"); 

    fpptr = fopen(file, "r");
    fptr = fopen(file1, "a");

    fflush(fpptr);
    fflush(fptr);
    
    while (fgets(buffer, 1024, fpptr) != NULL) {
        fprintf(fptr, "%s", buffer);
        fflush(fptr);
        fflush(fpptr);
    }

    
    while(fscanf(fpptr, "%s", buffer)==1){
        fflush(fptr);
        if(timestamp){
            fprintf(fptr, "%s \t ", buffer);
            timestamp = false;
            continue;
        }
            //timestamp = true;
        else if(!strcmp(buffer, "**")){
            fprintf(fptr, "**\n");
            timestamp = true;
        } else if(!strcmp(buffer, "***")){
            fprintf(fptr, "***\n");
            timestamp = true;
        }
        else{
            fprintf(fptr, "%s ", buffer);
        }
        fflush(fptr);
    }
    
    fclose(fpptr);
    fclose(fptr);

    remove(file);
    rename(file1, file);
}

*/
/*
void readbuffer(char* stringa){

        char scorre[1024];
        bool timestamp = true;
        int ch = 0;

        while(sscanf(stringa, "%s", scorre)==1){
            ch = strlen(scorre)+1;
            stringa += ch;
        if(timestamp){
            printf("%s\t", scorre);
            timestamp = false;
            continue;
        }
        if(!strcmp(scorre, ":")){
            continue;
        }
        if(!strcmp(scorre, "***")){
            printf("\n");
            timestamp = true;
        } else{
            printf("%s ", scorre);
        }
    }

}
*/
/*
void port_config(int sd, int port){

    char buffer[1024];
    char porta[1024];

    send_srv(sd, CMD_PORT);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send_srv(sd, username);
        break;
    }
    
    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        sprintf(porta, "%d", port);
        send_srv(sd, porta);
        break;
    }

    recv(sd, buffer, sizeof(buffer), 0);

    if(!strcmp(buffer, YES)){
        return;
    } else if(!strcmp(buffer, NO)){
        printf("ATTENZIONE ! Porta gia' utilizzata da un altro device!\n");
    }
    exit(1);

}
*/

// Questa funzione si occupa della registrazione del device
void reg_config(int sd){

    char buffer[1024];
    //char all[1024];

    send_srv(sd, CMD_REG);

    while(1){
        printf("- Inserisca un username:\n\n-> ");
        scanf("%s", username);
        if(!strcmp(username, "signup")){
            printf("\nATTENZIONE! Si prega di utilizzare un altro username.\n\n");
            continue;
        }
        //Mando l'username al server, lui controllera' se va bene
        send_srv(sd, username);
        // Ricevo la risposta se va bene o no
        recv(sd, buffer, sizeof(buffer), 0);
        // Se il server trova quell'username gia' in uso
        if(!strcmp(buffer, YES)){
            printf("\nATTENZIONE! Username gia' in uso.\n\n");
            continue;
        } else{
            break;
        }
    }

    while(1){
        printf("\n- Inserisca una password:\n\n-> ");
        scanf("%s", password);
        if(!strcmp(password, "signup")){
            printf("\nATTENZIONE! Si prega di utilizzare un'altra password.\n");
            continue;
        }
        break;
    }
    
    strcpy(buffer, username);
    // Invio al server l'username e la password insieme
    strcat(username, password);
    //printf("%s\n", username);
    send_srv(sd, username);
    strcpy(username, buffer);

}

// Questa funzione si occupa del login del device
bool log_config(int sd){

    char buffer[1024];
    char all[1024];
    int i = 0;

    send_srv(sd, CMD_LOG);

    printf("\n- Inserisca l'username:\n\n-> ");

    while(1){

        scanf("%s", username);
        i++;
        //printf("%d", i);
        //Mando l'username al server, lui controllera' se va bene
        send_srv(sd, username);

        if(!(strcmp(username, "signup")) && i){
            return true;
        }


        // Ricevo la risposta se va bene o no
        recv(sd, buffer, sizeof(buffer), 0);

        // Se il server NON trova quell'username
        if(!strcmp(buffer, NO)){
            printf("\nATTENZIONE! Username non esistente.\n\n");
            printf("- Si prega di inserire un username esistente.\n");
            printf("- Per creare un account digiti 'signup'.\n\n-> ");
            continue;
        } else{
            // Altrimenti registra l'username e il device lo comunica
            break;
        }
    }


    printf("\n- Inserisca la password:\n\n-> ");

    i = 0;

    while(1){
        
        scanf("%s", password);

        if(!(strcmp(password, "signup")) && i){
            send_srv(sd, password);
            return true;
        }
        //Mando password ed username al server, lui controllera' se va bene
        //sprintf(all, "%s%s", username, password);
        strcpy(all, username);
        // all = username.
        strcat(all, password);
        // all = usernamepassword.
        //printf("%s\n", all);
        send_srv(sd, all);
        // Ricevo la risposta se va bene o no
        recv(sd, buffer, sizeof(buffer), 0);

        i++;
        // Se il server NON trova quell'username
        if(!strcmp(buffer, NO)){
            printf("\nATTENZIONE! Password non corretta.\n\n");
            printf("- Si prega di inserire una password corretta.\n");
            printf("- Altrimenti digiti 'signup' per registrare un account.\n\n-> ");
            continue;
        } else{
            // Altrimenti registra l'username e il device lo comunica
            break;
        }
    }
    return false;

}

// Questa funzione si occupa della configurazione del device una volta loggato
void online_config(int sd, int port){

    char buffer[1024];
    time_t rawtime;
    struct tm * timeinfo;

    // Invio comando
    send_srv(sd, CMD_TMS);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send_srv(sd, username);
        break;
    }
    
    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }

        time(&rawtime);
        // Converto l'ora
        timeinfo = localtime(&rawtime);

        sprintf(timestamp, "%d-%d-%d|%d:%d:%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,
        timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        break;
    }

    send(sd, &port, sizeof(port), 0);

}

// Questa funzione si occupa della stampa del menu' a comparsa per i Device
void second_print(){
    printf("\nSi prega di inserire un comando:\n\n");
    printf("- hanging: per vedere gli utenti che hanno inviato messaggi mentre era offline.\n");
    printf("- show 'username': per ricevere i messaggi pendenti inviati da 'username'.\n");
    printf("- chat: per iniziare una chat.\n");
    printf("- out: per disconnettersi dal Server.\n\n-> ");
}

int grpchat_config(int sd, char users[1024]){

    struct sockaddr_in dev_addr;
    char buffer[1024];
    char lista[1024];
    char dev_usr[1024];
    int port;
    int dev_sd;
    int ret;

    send_srv(sd, CMD_RUBRIC);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send_srv(sd, users);
        break;
    }

    // Ricevo la lista
    recv(sd, lista, sizeof(lista), 0);

    if(!strcmp(lista, NO)){
        printf("\nATTENZIONE ! Nessun utente e' attualmente online.\n\n");
        return 0;
    }

    printf("\nSi prega di scegliere un utente da aggiungere.\n");
    printf("\nLista utenti online:\n\n%s\n", lista);

    // Diciamo al Server che username vogliamo aggiungere
    scanf("%s", dev_usr);

    send_srv(sd, dev_usr);

    strcat(users, dev_usr);
    strcat(users, "\n");

    recv(sd, buffer, sizeof(buffer), 0);

    port = atoi(buffer);

    dev_sd = dev_connect(&dev_addr, port);

    // Provo a connettermi al dispositivo
    ret = connect(dev_sd, (struct sockaddr*)&dev_addr, sizeof(dev_addr));

    if(ret < 0){
        // Dispositivo offline
        printf("\nATTENZIONE ! Il dispositivo non e' attualmente raggiungibile.\n\n");
        return 0;
    }

    //printf("\nInvio comando\n");
    send_dev(dev_sd, CMD_GRPCHAT);

    while(1){
        recv(dev_sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send_dev(dev_sd, username);
        break;
    }

    sprintf(grp_users, "%s %d", dev_usr, port);
    //printf("%s", grp_users);

    printf("In attesa che %s accetti la chat...\n", dev_usr);

    while(1){

        recv(dev_sd, buffer, sizeof(buffer), 0);

        if(!strcmp(buffer, RFD)){
            printf("%s aggiunto alla chat di gruppo !\n", dev_usr);
            strcpy(dev_users[num_users].usr, dev_usr);
            dev_users[num_users].port = port;
            dev_users[num_users].sd = dev_sd;
            num_users++;
            sprintf(buffer, "%d", my_port);
            send_dev(dev_sd, buffer);
            /*printf("num_users: %d\n", num_users);
            int j;
            for(j=0; j < num_users; j++){
                printf("user:%s\nport:%d.\n", dev_users[j].usr, dev_users[j].port);
            }*/
            break;
        }

        else if(!strcmp(buffer, NO)){
            printf("\nATTENZIONE ! %s ha rifiutato la chat !\n", dev_usr);
            return 0;
        }
    }

    char prova[1024];
    char invio[1024];
    int j;

    memset(invio, 0, sizeof(invio));
    memset(prova, 0, sizeof(prova));

    for(j=0; j < num_users; j++){
        //printf("%s %d\n", dev_users[j].usr, dev_users[j].port);
        sprintf(prova, "%s %d\n", dev_users[j].usr, dev_users[j].port);
        strcat(invio, prova);
    }

    while(1){
        recv(dev_sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send(dev_sd, invio, sizeof(invio), 0);
        break;
    }

    return dev_sd;

}

// Questa funzione si occupa della preparazione del messaggio da inviare nella chat
char* msg(){

    char timestamp[1024];
    char buffer[1024];
    char stringa[1024];

    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    // Converto l'ora
    timeinfo = localtime(&rawtime);
    sprintf(timestamp, "%d-%d-%d|%d:%d:%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,
    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    memset(messaggio, 0, sizeof(messaggio));

    strcpy(messaggio, "[");
    strcat(messaggio, timestamp);
    strcat(messaggio, "]");
    strcat(messaggio, " \t ");
    strcat(messaggio, username);
    strcat(messaggio, ": ");

    scanf("%s", stringa);

    if(!strcmp(stringa, EXIT)){
        return EXIT;
    }

    if(!strcmp(stringa, DELETE)){
        return DELETE;
    }

    if(!strcmp(stringa, ADD)){
        // Facciamo capire al Server che vogliamo la rubrica degli utenti online
        return ADD;
    }

    if(!strcmp(stringa, "share")){
        strcpy(messaggio, stringa);
        return messaggio;
    }

    fgets(buffer, sizeof(buffer), stdin);
    buffer[strlen(buffer)-1]= '\0';
    strcat(stringa, buffer);
    strcat(messaggio, stringa);

    return messaggio;

}

// Questa funzione si occupa della condivisione del file da parte di un mittente
void share_file(int sd){

    char filename[1024];
    char percorso[1024];
    char buffer[1024];
    char str[1024];
    FILE* fpptr;

    scanf("%s", filename);
    strcpy(percorso, username);
    strcat(percorso, "/");
    strcat(percorso, filename);
    fpptr = fopen(percorso, "r");
    if(!fpptr){
        printf("ATTENZIONE ! file %s inesistente.\n", filename);
        return;
    }
    send_dev(sd, SHARE);
    printf("Condivisione di %s in corso...\n\nContenuto:\n", filename);
    while(1){
        recv(sd, percorso, sizeof(percorso), 0);
        if(strcmp(percorso, RFD)){
           continue; 
        }
        send_dev(sd, filename);
        break;
    }

    memset(buffer, 0, sizeof(buffer));

    while (fgets(str, 1024, fpptr) != NULL) {
        strcat(buffer, str);
    }

    printf("%s\n", buffer);
    fclose(fpptr);

    while(1){
        recv(sd, percorso, sizeof(percorso), 0);
        if(strcmp(percorso, RFD)){
           continue; 
        }
        send_dev(sd, buffer);
        break;
    }

    printf("\nCondivisione file %s completata !\n", filename);
 
}

// Questa funzione aggiunge alla cronologia della chat i messaggi pendenti
void pendent_before_chat(int sd, char dev_usr[1024]){
    
    char buffer[1024];
    char percorso[1024];

    FILE* fptr;

    // Prima di iniziare la chat devo vedere se ci sono messsaggi pendenti
    // Senno' non torna la cronologia dei messaggi
    send_srv(sd, CMD_SHOW);


    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            send_srv(sd, username);
            break;
        }
    }

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            send_srv(sd, dev_usr);
            break;
        }
    }

    recv(sd, buffer, sizeof(buffer), 0);

    if(strcmp(buffer, NO)){
        strcpy(percorso, username);
        strcat(percorso, "/chat/");
        strcat(percorso, dev_usr);
        strcat(percorso, ".txt");

        fptr = fopen(percorso, "a");
        fflush(fptr);
        fprintf(fptr, "%s", buffer);
        fflush(fptr);
        fclose(fptr);
    }

}

// Questa funzione gestisce la chat quando un Device e' uscito e quindi e' andato offline
void offline_chat(int sd, char dev_usr[1024], FILE* fptr){

    char buffer[1024];
    send_srv(sd, CMD_CHATOFF);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send_srv(sd, dev_usr);
        break;
    }

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send_srv(sd, username);
        break;
    }

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        while(1){
            strcpy(buffer, msg());
            if(!strcmp(buffer, EXIT)){
                send_srv(sd, buffer);
                break;
            }
            printf("%s *\n", buffer);
            //strcat(buffer, " **");
            send_srv(sd, buffer);
            fflush(fptr);
            fprintf(fptr, "%s **\n", buffer);
            fflush(fptr);
            
        }
        break;
    }    

}  

// Questa funzione si occupa della chat dei Device
void chat(int sd, char dev_usr[1024]){

    fd_set master_chat;
    fd_set read_chat;
    int fdmax;
    int i;
    int j;
    char sent[1024];
    char coming[1024];
    char buffer[1024];
    char filename[1024];
    char users[1024];
    char buffer1[1024]; 
    int sockets[100];
    int ret;
    int new_sd;
    int num_sd = 0;
    int tot_users = num_users;
    struct sockaddr_in dev_addr;
    bool online = true;
    chatting = true;
    dev_sd = sd; 

    FD_ZERO(&read_chat);
    FD_ZERO(&master_chat);
    FD_SET(STDIN, &master_chat);
    FD_SET(sd, &master_chat);
    FD_SET(listener, &master_chat);

    fdmax = (sd > listener) ? sd : listener;
    sockets[num_sd++] = sd;

    FILE* fptr, *fpptr;

    strcpy(buffer, username);
    strcat(buffer, "/chat");
    mkdir(buffer, 0700);
    strcat(buffer, "/");
    strcat(buffer, dev_usr);
    strcat(buffer, ".txt");

    //pendent_before_chat(sd, dev_usr);

    printf("Chat con %s iniziata !\n\n", dev_usr);
    printf("--->");
    printf(" Per inviare un messaggio occorre digitare e premere invio.\n");
    printf("--->");
    printf(" Per aggiungere un partecipante digitare '\\u'\n");
    printf("--->");
    printf(" Per condividere un file digitare 'share nomefile'\n");
    printf("--->");
    printf(" Per eliminare la cronologia della chat '\\d'\n");
    printf("--->");
    printf(" Per uscire dalla chat digitare '\\q'\n\n");

    /*for(j=0; j < num_users; j++){
        printf("user:%s\nport:%d.\n", dev_users[j].usr, dev_users[j].port);
    }*/

    //printf("Il percorso e' %s\n", buffer);
    strcpy(buffer1, buffer);
    fptr = fopen(buffer, "r");
    //strcpy(chatt, filetobuffer(fptr));
    // Stampiamo la chat con la funzione filetobuffer()
    printf("%s", filetobuffer(fptr));
    fclose(fptr);
    //printf("Il percorso e' %s\n", buffer1);
    // Apriamo la chat in append per aggiornarla ad ogni messaggio
    fptr = fopen(buffer1, "a");
    fflush(fptr);

    strcat(users, username);
    strcat(users, "\n");
    strcat(users, dev_usr);
    strcat(users, "\n");

    /*printf("-> ");
    fflush(stdout);*/

    listen(listener, 10);

    while(1){
        read_chat = master_chat;
        fflush(stdout);
        select(fdmax + 1, &read_chat, NULL, NULL, NULL);

        for(i=0; i<=fdmax; i++) {
            // Cerco quelli pronti
            if(FD_ISSET(i, &read_chat)) {

                if(!i){
                    memset(sent, 0, sizeof(sent));
                    memset(coming, 0, sizeof(coming));

                    // Elaboriamo il messaggio
                    strcpy(sent, msg());

                    if(!strcmp(sent, DELETE)){
                        // Cancelliamo la chat
                        printf("Cancellazione chat in corso...\n\n");
                        fptr = fopen(buffer1, "w");
                        fflush(fptr);
                        break;
                    }

                    if(!strcmp(sent, "share")){
                        share_file(sd);
                        break;                 
                    }
                    
                    if(!strcmp(sent, ADD)){
                        new_sd = grpchat_config(srv_sd, users);
                        if(new_sd){
                            FD_SET(new_sd, &master_chat);
                            fdmax = (new_sd > fdmax) ? new_sd : fdmax; 
                            sockets[num_sd++] = new_sd;
                            /*sprintf(buffer, "%d", new_sd);
                            send_dev(new_sd, buffer);*/
                        }
                        break;
                    }

                    for(j=0; j < num_sd; j++){
                        send_dev(sockets[j], sent);
                    }

                    if(!strcmp(sent, EXIT)){
                        //printf("VOGLIO USCIRE\n\n");
                        chatting = false;
                        fclose(fptr);
                        return;
                    }


                    if(!online){
                        printf("%s *\n", sent);
                    } else{
                        printf("%s **\n", sent);
                    }

                    strcat(sent, " **");
                    fflush(fptr);   
                    fprintf(fptr, "%s\n", sent);
                    fflush(fptr);
                    /*printf("-> ");
                    fflush(stdout); */
                }
                else if(i==listener){

                    //printf("Listener ATTIVATO.\n");
                    int addrlen = sizeof(dev_addr);
                    new_sd = accept(listener, (struct sockaddr *)&dev_addr, (socklen_t*)&addrlen);
                    sockets[num_sd++] = new_sd;
                    //printf("num_sd: %d\nnew_sd: %d\n", num_sd, new_sd);

                    recv(new_sd, buffer, sizeof(buffer), 0);

                    char* estrai = buffer;
                    char scorri[1024];
                    int ch = 0;

                    sscanf(estrai, "%s", scorri);
                    strcpy(dev_users[num_users].usr, scorri);
                    printf("%s entrato nella chat !\n", scorri);
                    ch = strlen(scorri)+1;
                    estrai += ch;
                    strcat(users, scorri);
                    strcat(users, "\n");
                    sscanf(estrai, "%s", scorri);
                    dev_users[num_users].port = atoi(scorri);
                    dev_users[num_users].sd = new_sd;
                    num_users++;
                    tot_users++;

                    //printf("buffer: %s", buffer);

                    FD_SET(new_sd, &master_chat);                 // Aggiungo il nuovo socket al master
                    fdmax = (new_sd > fdmax) ? new_sd : fdmax;    // Aggiorno fdmax
                   
                }
                else {
                    
                    ret = recv(i, coming, sizeof(coming), 0);
                    
                    if(ret > 0){
                        fflush(stdin);
                        fflush(stdout);
                        if(!strcmp(coming, SHARE)){
                            send_dev(i, RFD);
                            recv(i, filename, sizeof(filename), 0);
                            printf("%s sta condividendo un file !\n", dev_usr);
                            strcpy(percorso, username);
                            strcat(percorso, "/");
                            strcat(percorso, filename);
                            send_dev(i, RFD);
                            recv(i, buffer, sizeof(buffer), 0);
                            fpptr = fopen(percorso, "a");
                            fflush(fpptr);
                            fprintf(fpptr, "%s", buffer);
                            fflush(fpptr);
                            printf("Contenuto di %s:\n%s\n", filename, buffer);
                            break;
                        }
                        else if(!strcmp(coming, EXIT)){
                            printf("ATTENZIONE ! %s e' uscito dalla chat.\n\n", dev_usr);
                            printf("I messaggi verranno comunque inoltrati a %s !\n", dev_usr);
                            online = false;
                            FD_CLR(i, &master_chat);
                            //close(i);
                            tot_users--;
                            if(!tot_users){
                                offline_chat(srv_sd, dev_usr, fptr);
                                return;
                            }
                        } 
                        else if(!strcmp(coming, GRPEXIT)){
                            for(j=0; j < num_users; j++){
                                if(dev_users[j].sd == i){
                                    printf("ATTENZIONE ! %s e' uscito dalla chat.\n", dev_users[j].usr);
                                    FD_CLR(i, &master_chat);
                                    tot_users--;
                                    close(i);
                                    break;
                                }
                            }
                        }
                        else{
                            printf("%s\n", coming);
                            //strcat(coming, " ***");
                            if(strcmp(sent, EXIT)){
                                fflush(fptr);
                                fprintf(fptr, "%s\n", coming);
                                fflush(fptr);
                            }
                        }
                        fflush(stdout);
                        fflush(stdin);
                        /*printf("-> ");
                        fflush(stdout);*/
                    }

                }
            }
        
        }
    }
}

// Questa funzione si occupa della chat di gruppo dei Device
void grp_chat(int sd, char dev_usr[1024], struct users chat_users[10]){

    fd_set master_chat;
    fd_set read_chat;
    int fdmax;
    int i;
    int j;
    int ret;
    int sockets[100];
    int new_sd;
    int num_sd = 0;
    int tot_users = num_users;
    struct sockaddr_in dev_addr;
    char buffer[1024];

    FD_ZERO(&read_chat);
    FD_ZERO(&master_chat);
    FD_SET(STDIN, &master_chat);
    FD_SET(sd, &master_chat);
    FD_SET(listener, &master_chat);

    fdmax = (sd > listener) ? sd : listener;

    sockets[num_sd++] = sd;
    fdmax = sd;

    for(i=1; i < num_users; i++){

        new_sd = dev_connect(&dev_addr, chat_users[i].port);
        // Provo a connettermi al dispositivo
        ret = connect(new_sd, (struct sockaddr*)&dev_addr, sizeof(dev_addr));
        if(ret < 0){
            // Dispositivo offline
            printf("\nATTENZIONE ! Il dispositivo non e' attualmente raggiungibile.\n\n");
            break;
        }
        chat_users[i].sd = new_sd;
        fdmax = (new_sd > sd) ? new_sd : sd;
        sprintf(buffer, "%s %d", username, my_port);
        send_dev(new_sd, buffer);
        FD_SET(new_sd, &master_chat);
        sockets[num_sd++] = new_sd;
    }
    
    //FILE* fptr, *fpptr;

    strcpy(buffer, username);
    strcat(buffer, "/chat");
    mkdir(buffer, 0700);
    strcat(buffer, "/");
    strcat(buffer, dev_usr);
    strcat(buffer, ".txt");

    char sent[1024];
    char coming[1024];

    printf("Chat di gruppo iniziata !\n\n");
    printf("--->");
    printf(" Per inviare un messaggio occorre digitare e premere invio.\n");
    /*printf("--->");
    printf(" Per aggiungere un partecipante digitare '\\u'\n");*/
    printf("--->");
    printf(" Per condividere un file digitare 'share nomefile'\n");
    printf("--->");
    printf(" Per eliminare la cronologia della chat '\\d'\n");
    printf("--->");
    printf(" Per uscire dalla chat digitare '\\q'\n\n");

    listen(listener, 10);

    while(1){
        read_chat = master_chat;
        fflush(stdout);
        select(fdmax + 1, &read_chat, NULL, NULL, NULL);

        for(i=0; i<=fdmax; i++) {
            if(FD_ISSET(i, &read_chat)) {

                if(!i){
                    memset(sent, 0, sizeof(sent));
                    memset(coming, 0, sizeof(coming));

                    strcpy(sent, msg());

                    if(!strcmp(sent, EXIT)){
                        chatting = false;
                        for(j=0; j < num_sd; j++){
                            send_dev(sockets[j], GRPEXIT);
                            //close(sockets[j]);
                        }
                        return;
                    }

                    for(j=0; j < num_sd; j++){
                        send_dev(sockets[j], sent);
                    }

                    if(!online){
                        printf("%s *\n", sent);
                    } else{
                        printf("%s **\n", sent);
                    }

                    strcat(sent, " **");

                }
                else if(i == listener){
                    //printf("Listener ATTIVATO.\n");
                    int addrlen = sizeof(dev_addr);
                    new_sd = accept(listener, (struct sockaddr *)&dev_addr, (socklen_t*)&addrlen);
                    //printf("num_sd: %d\nnew_sd: %d\n", num_sd, new_sd);
                    sockets[num_sd++] = new_sd;
                    recv(new_sd, buffer, sizeof(buffer), 0);

                    char* estrai = buffer;
                    char scorri[1024];
                    int ch = 0;

                    sscanf(estrai, "%s", scorri);
                    strcpy(chat_users[num_users].usr, scorri);
                    printf("%s entrato nella chat !\n", scorri);
                    ch = strlen(scorri)+1;
                    estrai += ch;
                    sscanf(estrai, "%s", scorri);
                    chat_users[num_users].port = atoi(scorri);
                    chat_users[num_users].sd = new_sd;
                    num_users++;
                    tot_users++;
                    //printf("num_sd: %d\nnew_sd: %d\n", num_sd, new_sd);

                    FD_SET(new_sd, &master_chat);                 // Aggiungo il nuovo socket al master
                    fdmax = (new_sd > fdmax) ? new_sd : fdmax;    // Aggiorno fdmax
                }
                else {
                    
                    ret = recv(i, coming, sizeof(coming), 0);
                    
                    if(ret > 0){
                        if(!strcmp(coming, GRPEXIT) || !strcmp(coming, EXIT)){
                            for(j=0; j < num_users; j++){
                                if(chat_users[j].sd == i){
                                    printf("ATTENZIONE ! %s e' uscito dalla chat.\n", chat_users[j].usr);
                                    FD_CLR(i, &master_chat);
                                    close(i);
                                    tot_users--;
                                    break;
                                }
                            }
                        }
                        else{
                            fflush(stdin);
                            fflush(stdout);
                            printf("%s\n", coming);
                            fflush(stdout);
                            fflush(stdin);
                        }
                    }

                    

                }
            }
        
        }
    }
}

// Questa funzione si occupa della configurazione pre-chat del Device
void chat_config(int sd){

    FILE* fptr;
    char buffer[1024];
    char dev_usr[1024];
    char lista[1024];
    struct sockaddr_in dev_addr;
    int dev_port;
    int dev_sd;
    int ret;
    //bool dev_friend = true;
    //bool dev_online = true;

    // Mandiamo il comando
    send_srv(sd, CMD_CHAT);

    // Mandiamo anche l'username ma prima chiediamo se il Server è ReadyForData
    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send_srv(sd, username);
        break;
    }

    // Ricevo la lista
    recv(sd, lista, sizeof(lista), 0);

    printf("Lista degli utenti registrati nel sistema:\n\n");
    printf("%s\n", lista);

    // Creo username/rubrica.txt

    strcat(percorso, "/");  
    strcat(percorso, "rubrica.txt");    

    // Creo/apro in lettura la rubrica
    fptr = fopen(percorso, "a+");
    fflush(fptr);
    if(!check_word(fptr, dev_usr)){
        // Cio' indica che e' la prima volta in cui il device chiede all'altro di comunicare
        //dev_friend = false;
    }
    fclose(fptr);

    while(1){
        printf("Si prega di inserire l'username con cui si vuole aprire una chat\n\n-> ");

        scanf("%s", dev_usr);

        if(!strcmp(dev_usr, username)){
            printf("ATTENZIONE ! Si prega di non inserire il proprio username.\n");
            continue;
        }

        send_srv(sd, dev_usr);
        recv(sd, buffer, sizeof(buffer), 0);

        if(!strcmp(buffer, NO)){
            printf("\nATTENZIONE ! Username non presente.\n");
            continue;
        }

        if(!strcmp(buffer, OFFLINE)){

            printf("Username offline, il messaggio verra' comunque inviato:\n\n-> ");
            strcpy(buffer, msg());
            //strcat(buffer, " *");
            send_srv(sd, buffer);
            strcpy(percorso, username);
            strcat(percorso, "/chat/");
            strcat(percorso, dev_usr);
            //strcpy(filename, percorso);
            strcat(percorso, ".txt");

            fptr = fopen(percorso, "a");
            fflush(fptr);
            fprintf(fptr, "%s **\n", buffer);
            fflush(fptr);
            fclose(fptr);
            return;

        }

        // A questo punto il Server ci ha confermato che il device contattato e' online

        // Il Server ci ha inviati la porta del Device
        dev_port = atoi(buffer);

        fptr = fopen(percorso, "a+");
        fflush(fptr);
        if(!check_word(fptr, dev_usr)){
            fprintf(fptr, "%s ", dev_usr);
            fprintf(fptr, "%s\n", buffer);
        }
        fclose(fptr);

        break;
    }

    /*
    // Prima di iniziare la chat devo vedere se ci sono messsaggi pendenti
    // Senno' non torna la cronologia dei messaggi

    send_srv(sd, CMD_SHOW);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            send_srv(sd, username);
            break;
        }
    }

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            send_srv(sd, dev_usr);
            break;
        }
    }

    recv(sd, buffer, sizeof(buffer), 0);

    if(strcmp(buffer, " ")){
        strcpy(percorso, username);
        strcat(percorso, "/chat/");
        strcat(percorso, dev_usr);
        strcat(percorso, ".txt");

        fptr = fopen(percorso, "a");
        fflush(fptr);
        fprintf(fptr, "%s", buffer);
        fflush(fptr);
        fclose(fptr);
    }
    */
    
    pendent_before_chat(sd, dev_usr);

    dev_sd = dev_connect(&dev_addr, dev_port);

    // Provo a connettermi al dispositivo
    ret = connect(dev_sd, (struct sockaddr*)&dev_addr, sizeof(dev_addr));

    if(ret < 0){
        // Dispositivo offline
        printf("Dispositivo offline\n");
        return;
        // dev_online = false;
    }
    //printf("\nInvio comando\n");
    send_dev(dev_sd, CMD_CHAT);

    while(1){
        recv(dev_sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send_dev(dev_sd, username);
        break;
    }

    srv_sd = sd;

    printf("In attesa che %s accetti la chat...\n", dev_usr);

    while(1){

        recv(dev_sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            sprintf(buffer, "%d", my_port);
            send(dev_sd, &buffer, sizeof(&buffer), 0);
            // Bisogna controllare se c'e' gia'
            strcpy(dev_users[num_users].usr, dev_usr);
            dev_users[num_users].port = dev_port;
            dev_users[num_users].sd = dev_sd;
            num_users++;
            /*printf("num_users: %d\n", num_users);
            int j=0;
            for(j=0; j < num_users; j++){
                printf("user:%s\nport:%d.\n", dev_users[j].usr, dev_users[j].port);
            }*/
            chat(dev_sd, dev_usr);
            break;
        }

        else if(!strcmp(buffer, NO)){
            strcpy(buffer, username);
            strcat(buffer, "/chat");
            mkdir(buffer, 0700);
            strcat(buffer, "/");
            strcat(buffer, dev_usr);
            strcat(buffer, ".txt");
            fflush(fptr);
            fptr = fopen(buffer, "a");
            fflush(fptr);
            printf("\nATTENZIONE ! %s ha rifiutato la chat !\n", dev_usr);
            printf("\nLa chat e' stata aperta, i messaggi verranno comunque inviati\n");
            offline_chat(srv_sd, dev_usr, fptr);
            break;
        }
    }


}

// Questa funzione si occupa del logout del Device
void out_config(int sd){

    char buffer[1024];

    send_srv(sd, CMD_OFF);

    // Bisogna utilizzare RFD
    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send_srv(sd, username);
        break;
    }

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send(sd, &my_port, sizeof(&my_port), 0);
        break;
    }

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send(sd, timestamp, sizeof(timestamp), 0);
        break;
    }

    strcat(percorso, "/rubrica.txt");

    //remove(percorso);
    //rmdir(pendent);
    //rmdir(username);
}

// Questa funzione si occupa del comando 'hanging' digitato dal Device
void hanging_config(int sd){

    char buffer[1024];

    send_srv(sd, CMD_HANGING);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            send_srv(sd, username);
            break;
        }
    }
    
    recv(sd, buffer, sizeof(buffer), 0);

    printf("%s", buffer);

    printf("\nDigitare qualsiasi cosa per presa visione.\n");

    scanf("%s", buffer);

}

// Questa funzione si occupa del comando 'show username' digitato dal Device
void show_config(int sd){

    char buffer[1024];
    char dev_usr[1024];
    char percorso[1024];
    //char filename[1024];
    FILE* fptr;

    send_srv(sd, CMD_SHOW);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            send_srv(sd, username);
            break;
        }
    }
    printf("Inserimento username...\n");

    scanf("%s", dev_usr);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            send_srv(sd, dev_usr);
            break;
        }
    }

    recv(sd, buffer, sizeof(buffer), 0);
    
    if(!strcmp(buffer, NO)){
        printf("ATTENZIONE ! L'username indicato non e' esistente.\n");
        printf("\nDigitare qualsiasi cosa per presa visione.\n");
        scanf("%s", buffer);
        return;
    }

    if(!strcmp(buffer, " ")){
        printf("\nNessun messaggio pendente dall'utente selezionato !\n");
        printf("\nDigitare qualsiasi cosa per presa visione.\n");
        scanf("%s", buffer);
        return;
    }
    
    //readbuffer(buffer);
    printf("%s", buffer);

    strcpy(percorso, username);
    strcat(percorso, "/chat/");
    strcat(percorso, dev_usr);
    //strcpy(filename, percorso);
    strcat(percorso, ".txt");

    fptr = fopen(percorso, "a");
    fflush(fptr);
    fprintf(fptr, "%s", buffer);
    fflush(fptr);
    fclose(fptr);

    printf("\nDigitare qualsiasi cosa per presa visione.\n");

    scanf("%s", buffer);

}

// Handler per la gestione di segnali di uscita improvvisa come CTRL+C o CTRL+Z
void handler(int sig){
    
    //printf("Entro nell'handler\n");

    if(chatting){
        send_dev(dev_sd, EXIT);
        printf("Il Device sta terminando ...\n");
    }

    if(online){
        out_config(srv_sd);
        printf("\n");
        exit(0);
    }
}
void handler1(int sig){
    exit(-1);
}

// Main
int main(int argc, char* argv[]){

    int ret, srv_port;
    struct sockaddr_in my_addr, srv_addr, dev_addr;
    char spacenter[1024];
    fd_set master;                                  // Set principale gestito dal programmatore con le macro 
    fd_set read_fds;                                // Set di lettura gestito dalla select 
    int fdmax;                                      // Numero max di descrittori
    int newfd;                                      // Socket di comunicazione
    char buffer[1024];
    char command[1024];
    char dev_usr[1024];
    char dev_port[1024];
    char wait[1024];
    int i;
    int addrlen;

    if(argc < 2){
        printf("ATTENZIONE! Inserisca una porta.\n");
        exit(-1);
    }

    // Porta passata dalla linea di comando
    my_port = atoi(argv[1]);

    // Si connette il device alla porta desiderata 
    listener = ip_config(&my_addr, my_port);

    listen(listener, 10);

    printf("\nConnessione al server in corso...\n");

    while(1){
        printf("\nSi prega di inserire la porta del Server a cui si vuole accedere:\n\n-> ");
        //scanf("%d", &srv_port);
        srv_port=4242;
        // Configuriamo il socket connesso al server
        srv_sd = srv_config(&srv_addr, srv_port);
        // Connettiamo il Device al Server
        ret = connect(srv_sd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
        if(ret < 0){
            printf("\nATTENZIONE! Server offline sulla porta %d.\n\n--> La porta 4242 e' quella di default.\n", srv_port);
            continue;
        }
        
        //port_config(srv_sd, my_port);

        break;
    }

    // Azzero i set
    FD_ZERO(&master);
    FD_ZERO(&read_fds); 
    
    // Aggiungo il listener al set master
    //FD_SET(listener, &master);
    // Aggiungo il descrittore della STDIN al set master
    FD_SET(STDIN, &master);
    FD_SET(listener, &master);

    // Il socket maggiore
    fdmax = listener;

    // Prima stampa
    first_print();
    signal(SIGTSTP, handler1);   // CTRL+Z o chiusura terminale

    //printf("\nDevice connesso al server\n");  

    printf("\n\n---> Digiti 'signup' per creare un account.\n\n");
    printf("---> Se ha gia' un account registrato digiti 'login'.\n\n-> ");

    while(1){       
        
        scanf("%s", spacenter);
        if(!strcmp(spacenter, "signup") || !strcmp(spacenter,"SIGNUP")){
            printf("\n---> Registrazione account in corso...\n\n");
            // La funzione si occupa di tutta la fase di registrazione comprensiva di richiesta di username e password
            reg_config(srv_sd);
            printf("\n---> Il suo account e' stato registrato correttamente\n\n");
            break;

        } else if(!strcmp(spacenter, "login") || !strcmp(spacenter, "LOGIN")){
            // printf("Gestione login\n");
            // La funzione si occupa di tutta la fase di login
            // E' una booleana: se ritorna true vuol dire che l'utente vuole fare la signup
            if(log_config(srv_sd)){
                    printf("\n---> Registrazione account in corso...\n\n");
                    // La funzione si occupa di tutta la fase di registrazione comprensiva di richiesta di username e password
                    reg_config(srv_sd);
                    printf("\n---> Il suo account e' stato registrato correttamente\n\n");
            }
            break;
        }
        else{
            printf("\nATTENZIONE ! Comando -%s- non riconosciuto.\n", spacenter);
            printf("\n--> Digiti 'signup' per creare un account.\n\n");
            printf("--> Se ha gia' un account registrato digiti 'login'.\n\n-> ");
            continue;
        }
    }


    // Adesso il Device e' online, dobbiamo inviare al Server il timestamp corrente
    online_config(srv_sd, my_port);

    online = true;

    // Handler per la gestione della disconnessione del device improvvisa
    signal(SIGINT, handler);    // CTRL+C

    printf("Salve %s ! Benvenuto nel sistema di chatting.\n", username);

    // Creiamo la cartella dell'username
    mkdir(username, 0700);
    strcpy(pendent, username);
    strcat(pendent, "/pendent");
    rmdir(pendent);
    //mkdir(pendent, 0700);

    // Il Device si e' loggato, bisogna creare il menu' di comparsa
    second_print();

    while(1){

        strcpy(percorso, username);
        //strcat(percorso, "/");
        read_fds = master;
        fflush(stdout);

        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        for(i=0; i<=fdmax; i++) {
            // Cerco quelli pronti
            if(FD_ISSET(i, &read_fds)) {
                if(!i){
                    scanf("%s", buffer);
                    printf("Gestione comando %s\n", buffer);

                    if(!strcmp(buffer, "hanging")){
                        // Gestione comando hanging
                        //printf("Gestione comando %s\n", buffer);
                        hanging_config(srv_sd);
                        second_print();
                    }

                    else if(!strcmp(buffer, "show")){
                        // Gestione comando show
                        show_config(srv_sd);
                        second_print();
                    }

                    else if(!strcmp(buffer, "chat")){
                        // Gestione comando chat
                        //printf("Gestione comando %s\n", buffer);
                        chat_config(srv_sd);
                        second_print();
                    }

                    else if(!strcmp(buffer, "out")){
                        // Gestione comando out
                        out_config(srv_sd);
                        exit(0);
                    }

                    else{
                        printf("\nATTENZIONE ! Comando -%s- inesistente.\n", buffer);
                        second_print();
                    }

                }

                else if(i == listener) {                    // Se quello pronto e' il listener
  
                    addrlen = sizeof(dev_addr);
                    //struct sockaddr_in *prova = &dev_addr;
                    newfd = accept(listener, (struct sockaddr *)&dev_addr, (socklen_t*)&addrlen);

                    /*printf("Listener: aggiungo nuovo socket al set\n");
                    printf("%d\n", (&dev_addr)->sin_port);*/

                    FD_SET(newfd, &master);                     // Aggiungo il nuovo socket al master
                    fdmax = (newfd > fdmax) ? newfd : fdmax;    // Aggiorno fdmax
                   
                }
                else{                                     // Se il socket pronto e' il comunicatore

                while(1){
                    ret = recv(i, command, sizeof(command), 0);   

                    if(!ret){                               // Socket i e' stato chiuso, Device offline
                        //printf("Socket chiuso\n");
                        FD_CLR(i, &master);                 // Lo tolgo dal master 
                        close(i);                           // Lo chiudo
                        break;

                    } 
                    else if(ret > 0){                     // Qui arriva il SEGNALE /XXX

                        //printf("Il comunicatore (socket %d) e' pronto\n", i);
                        //printf("E' arrivato il comando %s\n", command);

                        // Gestione registrazione
                        if(!strcmp(command, CMD_CHAT)){

                            send_dev(i, RFD);

                            recv(i, dev_usr, sizeof(dev_usr), 0);

                            printf("%s vuole iniziare una chat con te !\n\n", dev_usr);

                            printf("---> Digita qualsiasi cosa per entrare nella chat.\n");
                            printf("---> Se si vuole rifutare la chat digitare '\\q'.\n\n");

                            scanf("%s", wait);


                            if(!strcmp(wait, EXIT)){
                                send_dev(i, NO);
                                //close(i);
                                break;
                            }

                            //buffer contiene dev_usr
                            pendent_before_chat(srv_sd, dev_usr);

                            send_dev(i, RFD);

                            recv(i, dev_port, sizeof(dev_port), 0);

                            strcpy(dev_users[num_users].usr, dev_usr);
                            dev_users[num_users].port = atoi(dev_port);
                            dev_users[num_users].sd = i;
                            //printf("USR: %s\nPORT: %d\n", dev_users[num_users].usr, dev_users[num_users].port);
                            num_users++;

                            //printf("Gestione CHAT\n");
                            //printf("Hai un nuovo messaggio !\n");
                            chat(i, dev_usr);
                            second_print();
                        }
                        else if(!strcmp(command, CMD_GRPCHAT)){

                            struct users chat_user[10];
                            num_users = 0;

                            send_dev(i, RFD);

                            recv(i, dev_usr, sizeof(dev_usr), 0);

                            printf("%s ti ha invitato ad una chat di gruppo !\n\n", dev_usr);

                            printf("---> Digita qualsiasi cosa per entrare nella chat.\n");
                            printf("---> Se si vuole rifutare la chat digitare '\\q'.\n\n");

                            scanf("%s", wait);

                            if(!strcmp(wait, EXIT)){
                                send_dev(i, NO);
                                break;
                            }

                            send_dev(i, RFD);

                            recv(i, dev_port, sizeof(dev_port), 0);
                            
                            strcpy(chat_user[num_users].usr, dev_usr);
                            chat_user[num_users].port = atoi(dev_port);
                            chat_user[num_users].sd = i;
                            //printf("user:%s\nport:%d.\n", chat_user[num_users].usr, chat_user[num_users].port);
                            num_users++;

                            memset(buffer, 0, sizeof(buffer));

                            send_dev(i, RFD);
                            //Ricevo la lista 
                            recv(i, buffer, sizeof(buffer), 0);
                           
                            //printf("Lista:%s", buffer);
                            
                            char* estrai = buffer;
                            char scorri[1024];
                            int ch = 0;

                            memset(scorri, 0, sizeof(scorri));

                            while(sscanf(estrai, "%s", scorri)==1){
                                if(strcmp(scorri, username)){
                                    //printf("USR: %s\n", scorri);
                                    strcpy(chat_user[num_users].usr, scorri);
                                    //printf("USR: %s\n", chat_user[num_users].usr);
                                    ch = strlen(scorri)+1;
                                    estrai += ch;
                                    sscanf(estrai, "%s", scorri);
                                    //printf("PORT: %d\n", atoi(scorri));
                                    chat_user[num_users].port = atoi(scorri);
                                    //printf("PORT: %d\n", chat_user[num_users].port);
                                    num_users++;
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

                            int j;
                            for(j=0; j < num_users; j++){
                                printf("user:%s\nport:%d.\n", chat_user[j].usr, chat_user[j].port);
                            }

                            grp_chat(i, dev_usr, chat_user);

                            second_print();
                        }
                    } 
                    else{
                        perror("Errore nella reiceve: ");
                        break;
                    }
                    break;
                } 
                }
            } 
        }

    //second_print();
    }

    //sleep(60);

    return 0;

}
