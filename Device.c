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

// Lista di comandi
#define CMD_CHAT "/CHAT\0"
#define CMD_CHATOFF "/CHATOFF\0"
#define CMD_GRPCHAT "/GRPCHAT\0"
#define CMD_HANGING "/HANGING\0"
#define CMD_LOG "/LOGIN\0"
#define CMD_OFF "/OFF\0"
#define CMD_PORT "/PORT\0"
#define CMD_REG "/REG\0"
#define CMD_RUBRIC "/RUBRIC\0"
#define CMD_SHOW "/SHOW\0"
#define CMD_TMS "/TIMESTAMP\0"

// Lista di espressioni
#define ADD "\\u\0"
#define DELETE "\\d\0"
#define EXIT "\\q\0"
#define GRPEXIT "/GRPEXIT\0"
#define YES "/YES\0"
#define NO "/NO\0"
#define OFFLINE "/OFFLINE\0"
#define RFD "RFD\0"
#define SHARE "/SHARE\0"
#define SRV_OUT "/SRV_OUT\0"
#define STDIN 0

char username[1024];                    // username del device
char password[1024];                    // password del device
char timestamp[1024];                   // username*timestamp*porta del device
char percorso[1024];                    // percorso: username/file.txt
char rubrica[1024];                     // rubrica
char pendent[1024];
char entry[1024];                       // messaggio di entrata nella chat
char sharing[1024];                     // messaggio di share nella chat
char messaggio[1024];                   // messaggio della chat
char all_chat[1024];                    // variabile globale per la funzione "filetobuffer", contiene la cronologia della chat
char grp_users[1024];                   // variabile contenente gli username e la loro porta che il Device ha invitato nella chat di gruppo

int my_port;                            // porta del device
int srv_sd;                             // communication socket descriptor (Server)
int dev_sd;                             // communication socket descriptor (Device)
int listener;                           // Socket di ascolto
bool online = false;                    // se true, Device online
bool chatting = false;                  // se true, Device dentro la chat
bool server_out = false;

int num_users = 0;

// Struttura dati che memorizza i componenti di una chat con tutte le informazioni del caso
struct users{
    char usr[1024];     // Username del device
    int port;           // Porta del device
    int sd;             // SocketDescriptor
};

struct users dev_users[10];     // Vettore della struttura dati

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

    memset(all_chat, 0, sizeof(all_chat));         // Pulizia della variabile globale
    while (fgets(scorre, 1024, fptr) != NULL) {
        strcat(all_chat, scorre);
        fflush(fptr);
    }
    
    return all_chat;

}

// Questa funzione si occupa della configurazione di rete del Device
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

// Questa funzione si occupa della configurazione di rete al Server
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

// Questa funzione si occupa della connessione ad un Device
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

    send(sd, cmd, strlen(cmd)+1, 0);
    
}

int send_dev(int sd, char* cmd){
    
    int ret;
    //printf("Invio %s\n", cmd);
    ret = send(sd, cmd, strlen(cmd)+1, 0);

    return ret;
}

// Questa funzione si occupa della registrazione del device
void reg_config(int sd){

    char buffer[1024];

    // Mandiamo al Server il comando
    send_srv(sd, CMD_REG);

    while(1){
        printf("- Inserisci un username:\n\n-> ");

        // Chiediamo a video l'username
        scanf("%s", username);

        if(!strcmp(username, "signup")){
            printf("\nATTENZIONE! Si prega di utilizzare un altro username.\n\n");
            continue;
        }

        // Mando l'username al Server, lui controllera' se va bene
        send_srv(sd, username);

        // Ricevo la risposta se va bene o no
        recv(sd, buffer, sizeof(buffer), 0);

        // Se il Server trova quell'username gia' in uso manda YES
        if(!strcmp(buffer, YES)){
            printf("\nATTENZIONE! Username gia' in uso.\n\n");
            // Si riparte
            continue;
        } else{
            // Altrimenti abbiamo finito per quanto riguarda l'username
            break;
        }
    }

    while(1){
        printf("\n- Inserisci una password:\n\n-> ");

        // Si chiede la password
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
    send_srv(sd, username);

    strcpy(username, buffer);

}

// Questa funzione si occupa del login del device
bool log_config(int sd){

    char buffer[1024];
    char all[1024];
    int i = 0;

    send_srv(sd, CMD_LOG);

    printf("\n- Inserisci lo username:\n\n-> ");

    while(1){

        scanf("%s", username);

        i++;

        // Mando l'username al server, lui controllera' se va bene
        send_srv(sd, username);

        // Se l'utente digita signup vuol dire che si vuole registrare
        if(!(strcmp(username, "signup")) && i){
            // In questo caso si ritorna true, vedremo nel main che verra' chiamata la reg_config()
            return true;
        }

        // Ricevo la risposta se va bene o no
        recv(sd, buffer, sizeof(buffer), 0);

        // Se il server NON trova quell'username
        if(!strcmp(buffer, NO)){
            printf("\nATTENZIONE! Username non esistente.\n\n");
            printf("- Si prega di inserire un username esistente.\n");
            printf("- Per creare un account digita 'signup'.\n\n-> ");
            continue;
        } else{
            // Altrimenti registra l'username e il device lo comunica
            break;
        }
    }


    printf("\n- Inserisci la password:\n\n-> ");

    i = 0;

    while(1){
        
        scanf("%s", password);

        if(!(strcmp(password, "signup")) && i){
            send_srv(sd, password);
            return true;
        }
        
        // Mando password ed username al server, lui controllera' se va bene
        strcpy(all, username);
        // all = username.
        strcat(all, password);
        // all = usernamepassword.
        send_srv(sd, all);
        // Ricevo la risposta se va bene o no
        recv(sd, buffer, sizeof(buffer), 0);

        i++;

        // Se il server NON trova quell'username
        if(!strcmp(buffer, NO)){
            printf("\nATTENZIONE! Password non corretta.\n\n");
            printf("- Si prega di inserire una password corretta.\n");
            printf("- Altrimenti digita 'signup' per registrare un account.\n\n-> ");
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
    //port = htons(port);

    // Invio comando al Server
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
    printf("- chat 'username': per iniziare una chat con 'username'.\n");
    printf("- out: per disconnettersi dal Server.\n\n-> ");
}

// Questa funzione si occupa della preparazione del messaggio di entrata di un Device in una chat
char* entry_msg(char dev_usr[1024]){

    char timestamp[1024];

    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    // Converto l'ora
    timeinfo = localtime(&rawtime);
    sprintf(timestamp, "%d-%d-%d|%d:%d:%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,
    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    memset(entry, 0, sizeof(entry));

    strcpy(entry, "[");
    strcat(entry, timestamp);
    strcat(entry, "]");
    strcat(entry, " \t ");
    strcat(entry, dev_usr);
    strcat(entry, " e' entrato nella chat !\n");

    return entry;

}

// Questa funzione si occupa della preparazione del messaggio di share di un Device in una chat
char* share_msg(char dev_usr[1024]){

    char timestamp[1024];

    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    // Converto l'ora
    timeinfo = localtime(&rawtime);
    sprintf(timestamp, "%d-%d-%d|%d:%d:%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,
    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    memset(sharing, 0, sizeof(sharing));

    strcpy(sharing, "[");
    strcat(sharing, timestamp);
    strcat(sharing, "]");
    strcat(sharing, " \t ");
    strcat(sharing, dev_usr);
    strcat(sharing, " ha condiviso un file !\n");
    
    return sharing;

}

// Questa funzione si occupa della configurazione della chat di gruppo
int grpchat_config(int sd, char users[1024], char usr[1024]){

    struct sockaddr_in dev_addr;
    char buffer[1024];
    char lista[1024];
    char dev_usr[1024];
    char prova[1024];
    char invio[1024];
    int j;
    int i;
    int port;
    int dev_sd;
    int ret;

    // Mando il comando al Server
    send_srv(sd, CMD_RUBRIC);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        // Invio la lista di users della chat
        send_srv(sd, users);
        break;
    }

    // Ricevo la lista di quelli online
    recv(sd, lista, sizeof(lista), 0);

    if(!strcmp(lista, NO)){
        printf("\nATTENZIONE ! Nessun utente e' attualmente online.\n\n");
        return 0;
    }

    printf("\nSi prega di scegliere un utente da aggiungere.\n");
    printf("\nLista utenti online:\n\n%s\n", lista);

    // Diciamo al Server che username vogliamo aggiungere
    scanf("%s", dev_usr);

    for(i=1; i < num_users; i++){
        if(!strcmp(dev_usr, usr) || !strcmp(dev_usr, dev_users[i].usr)){
            printf("ATTENZIONE ! Username indicato gia' presente in chat !\n");
            send_srv(sd, dev_usr);
            recv(sd, buffer, sizeof(buffer), 0);
            return 0;
        }
    }


    // Mandiamo al Server l'username
    send_srv(sd, dev_usr);

    // Ricevo dal server la porta del Device
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

    // Invio comando al Device
    send_dev(dev_sd, CMD_GRPCHAT);

    while(1){
        recv(dev_sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        // Invio al Device l'username
        send_dev(dev_sd, username);
        break;
    }

    printf("In attesa che %s accetti la chat...\n", dev_usr);

    while(1){
        recv(dev_sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            // Il device ha accettato la richiesta di chat
            // Aggiorniamo la struttura dati
            strcpy(dev_users[num_users].usr, dev_usr);
            dev_users[num_users].port = port;
            dev_users[num_users].sd = dev_sd;
            // Aggiungiamo alla lista degli users il Device
            strcat(users, dev_usr);
            strcat(users, "\n");
            num_users++;
            // Stampiamo l'entry message
            printf("%s", entry_msg(dev_usr));
            sprintf(buffer, "%d", my_port);
            // Inviamo al device la porta
            send_dev(dev_sd, buffer);
            break;
        }
        else if(!strcmp(buffer, NO)){
            printf("\nATTENZIONE ! %s ha rifiutato la chat !\n", dev_usr);
            return 0;
        }
    }

    memset(invio, 0, sizeof(invio));
    memset(prova, 0, sizeof(prova));

    // Prepariamo una lista con tutte le informazioni dei componenti della chat
    for(j=0; j < num_users; j++){
        sprintf(prova, "%s %d\n", dev_users[j].usr, dev_users[j].port);
        strcat(invio, prova);
    }

    while(1){
        recv(dev_sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        // Inviamo le informazioni al device, che le estrarra' e le salvera' nella sua struttura dati
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
    // [timestamp]   username: messaggio

    scanf("%s", stringa);

    // I primi 4 if rappresentano la richiesta da parte del Device di un comando specifico della chat

    // \q, uscita dalla chat
    if(!strcmp(stringa, EXIT)){
        return EXIT;
    }

    // \d, cancellazione della cronologia della chat
    if(!strcmp(stringa, DELETE)){
        return DELETE;
    }

    // \u, richiesta di aggiunta di un componente
    if(!strcmp(stringa, ADD)){
        return ADD;
    }

    // share, richiesta di condivisione del file
    if(!strcmp(stringa, "share")){
        strcpy(messaggio, stringa);
        return messaggio;
    }

    // In questo caso abbiamo un messaggio che memorizziamo tramite la fgets()
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
    int j = 0;
    FILE* fpptr;

    // Chiediamo a video il nome del file
    scanf("%s", filename);
    strcpy(percorso, username);
    strcat(percorso, "/");
    strcat(percorso, filename);
    // Apriamo il file
    fpptr = fopen(percorso, "r");

    // Non esiste
    if(!fpptr){
        printf("ATTENZIONE ! file %s inesistente.\n", filename);
        return;
    } 

    // Inviamo il comando di sharing a tutti i componenti del gruppo
    for(j=0; j < num_users; j++){
        send_dev(dev_users[j].sd, SHARE);
    }

    //send_dev(sd, SHARE);

    printf("Condivisione di %s in corso...\n\nContenuto:\n", filename);
    for(j=0; j < num_users; j++){
        while(1){
            recv(dev_users[j].sd, percorso, sizeof(percorso), 0);
            if(strcmp(percorso, RFD)){
                continue; 
            }
            // Inviamo a tutti i componenti della chat il nome del file condiviso
            send_dev(dev_users[j].sd, filename);
            break;
        }
    }

    memset(buffer, 0, sizeof(buffer));

    while (fgets(str, 1024, fpptr) != NULL) {
        // Copiamo dentro buffer il contenuto del file
        strcat(buffer, str);
    }

    printf("%s\n", buffer);
    fclose(fpptr);

    for(j=0; j < num_users; j++){
        while(1){
            recv(dev_users[j].sd, percorso, sizeof(percorso), 0);
            if(strcmp(percorso, RFD)){
                continue; 
            }
            // Inviamo il contenuto del file ai membri del gruppo
            send_dev(dev_users[j].sd, buffer);
            break;
        }
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
    // Mando la richiesta di SHOW al Server
    send_srv(sd, CMD_SHOW);


    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            // Invio il mio username
            send_srv(sd, username);
            break;
        }
    }

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            // Invio l'username del Device appartenente alla chat interessata
            send_srv(sd, dev_usr);
            break;
        }
    }

    recv(sd, buffer, sizeof(buffer), 0);

    if(strcmp(buffer, NO)){

        // Appendiamo i messaggi pendenti a quelli vecchi
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
    
    // Inviamo il comando al Server
    send_srv(sd, CMD_CHATOFF);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        // Inviamo al Server l'username del Device della chat offline
        send_srv(sd, dev_usr);
        break;
    }

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        // Inviamo al Server il mio username
        send_srv(sd, username);
        break;
    }

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        while(1){
            // Prepariamo il messaggio da inviare
            strcpy(buffer, msg());

            // Se e' '\q' diciamo al Server di uscire
            if(!strcmp(buffer, EXIT)){
                send_srv(sd, buffer);
                break;
            }

            // Stampiamo a video il messaggio
            printf("%s *\n", buffer);
            // Mandiamo al Server il messaggio
            send_srv(sd, buffer);
            fflush(fptr);
            // Copiamo nella chat il messaggio
            fprintf(fptr, "%s **\n", buffer);
            fflush(fptr);
        }
        break;
    }    

}  

// Questa funzione si occupa della chat dei Device
void chat(int sd, char dev_usr[1024], bool grp){

    fd_set master_chat;
    fd_set read_chat;
    int fdmax;
    int i;
    int j;
    int y;
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
    bool exiter = false;
    chatting = true;
    dev_sd = sd; 
    int port_connected[10];

    // In questa funzione utilizziamo la funzione select e quindi anche i set dei descrittori
    FD_ZERO(&read_chat);
    FD_ZERO(&master_chat);
    FD_SET(STDIN, &master_chat);        // StandardInput
    FD_SET(sd, &master_chat);           // Socket Descriptor di comunicazione con l'utente che mi ha invitato alla chat
    FD_SET(listener, &master_chat);     // Listener
    FD_SET(srv_sd, &master_chat);       // Socket Desctiptor del Server

    fdmax = (sd > listener) ? sd : listener;
    sockets[num_sd++] = sd;

    // Se si tratta di un invito in una chat di gruppo, l'utente invitato deve collegarsi con tutti i Device
    // presenti nella chat. Tutte le informazioni utili sono contenute nella struttura dati dev_users.
    if(grp){
        printf("Chat con %s", dev_usr);
        y=0;
        for(i=1; i < num_users; i++){
            for(j=0; j < y ;j++){
                if(dev_users[i].port == port_connected[j]){
                    exiter = true;
                }
            }
            if(exiter){
                exiter = false;
                continue;
            }
            // i connette al dispositivo
            new_sd = dev_connect(&dev_addr, dev_users[i].port);
            port_connected[y++]=dev_users[i].port;
            //printf("Mi connetto a %s\nPorta: %d\nsd: %d\n", dev_users[i].usr, dev_users[i].port, dev_users[i].sd);
            // Provo a connettermi al dispositivo
            ret = connect(new_sd, (struct sockaddr*)&dev_addr, sizeof(dev_addr));
            if(ret < 0){
                // Dispositivo offline
                printf("\nATTENZIONE ! Il dispositivo non e' attualmente raggiungibile.\n\n");
                continue;
            }
            dev_users[i].sd = new_sd;
            fdmax = (new_sd > sd) ? new_sd : sd;
            sprintf(buffer, "%s %d", username, my_port);
            // Mandiamo al Device a cui ci siamo collegati le nostre informazioni così che le salvi nella sua struttura dati
            send_dev(new_sd, buffer);
            // Inseriamo nel set il descrittore
            FD_SET(new_sd, &master_chat);
            strcat(users, dev_users[i].usr);
            if(i == num_users-1){
                printf(" e %s", dev_users[i].usr);   
            } else{
                printf(", %s", dev_users[i].usr);   
            }
            strcat(users, "\n");
            // Mettiamo nel vettore dei socket il nuovo descrittore
            sockets[num_sd++] = new_sd;
        }
        printf(" iniziata !\n\n");
    }

    FILE* fptr, *fpptr;

    // buffer -> username
    strcpy(buffer, username);
    // buffer -> username/chat
    strcat(buffer, "/chat");
    // Creiamo cartella username/chat
    mkdir(buffer, 0700);
    strcat(buffer, "/");
    strcat(buffer, dev_usr);
    strcat(buffer, ".txt");
    // buffer -> username/chat/dev_usr.txt

    // Stampiamo il menù della chat
    if(!grp){
        printf("Chat con %s iniziata !\n\n", dev_usr);
    }
    printf("--->");
    printf(" Per inviare un messaggio occorre digitarlo e premere invio.\n");
    printf("--->");
    printf(" Per aggiungere un partecipante digitare '\\u'.\n");
    printf("--->");
    printf(" Per condividere un file digitare 'share \"nomefile\"'.\n");
    printf("--->");
    printf(" Per eliminare la cronologia della chat '\\d'.\n");
    printf("--->");
    printf(" Per uscire dalla chat digitare '\\q'.\n\n");

    // Backup del buffer
    strcpy(buffer1, buffer);
    fptr = fopen(buffer, "a+");

    // Stampiamo la chat con la funzione filetobuffer()
    printf("%s", filetobuffer(fptr));
    fclose(fptr);

    // Apriamo la chat in append per aggiornarla ad ogni messaggio
    fptr = fopen(buffer1, "a");
    fflush(fptr);

    // Aggiungiamo alla lista degli users l'username e il dev_usr
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

                if(!i){ // STDIN
                    memset(sent, 0, sizeof(sent));
                    memset(coming, 0, sizeof(coming));

                    // Elaboriamo il messaggio
                    strcpy(sent, msg());
                    
                    // Se un partecipante invitato in una chat di gruppo vuole uscire
                    if(!strcmp(sent, EXIT) && grp){
                        chatting = false;
                        // Manda a tutti i componenti il comando GRPEXIT
                        for(j=0; j < num_sd; j++){
                            send_dev(sockets[j], GRPEXIT);
                        }
                        // Azzera la struttura dati e il contatore
                        memset(dev_users, 0, sizeof(dev_users));
                        num_users=0;
                        return;
                    }

                    // Comando di cancellazione della chat
                    if(!strcmp(sent, DELETE)){
                        // Cancelliamo la chat
                        printf("\nCancellazione chat completata !\n\n");
                        fptr = fopen(buffer1, "w");
                        fflush(fptr);
                        break;
                    }
                    
                    // Comando di share di un file
                    if(!strcmp(sent, "share")){
                        share_file(sd);
                        break;                 
                    }

                    // Comando di aggiunta di un componente
                    if(!strcmp(sent, ADD)){
                        new_sd = grpchat_config(srv_sd, users, dev_usr);
                        if(new_sd){
                            // Se ritorna un descriptor != 0, allora lo aggiungiamo a tutti i set
                            FD_SET(new_sd, &master_chat);
                            fdmax = (new_sd > fdmax) ? new_sd : fdmax; 
                            sockets[num_sd++] = new_sd;
                            tot_users++;
                            fflush(fptr);   
                            // Inseriamo nella chat il messaggio di entry (entry e' una variabile globale)
                            fprintf(fptr, "%s", entry);
                            fflush(fptr);
                        }
                        break;
                    }

                    // Se arriviamo qui vuol dire che siamo in un messaggio normale
                    for(j=0; j < num_users; j++){
                        // Lo inviamo a tutti
                        send_dev(dev_users[j].sd, sent);
                    }

                    // In questo caso e' un componente della chat "normale" 
                    if(!strcmp(sent, EXIT)){
                        chatting = false;
                        fclose(fptr);
                        // Se il Sever e' offline si esce
                        if(server_out){
                            printf("ATTENZIONE ! Il server e' offline, l'esecuzione viene interrotta.\n");
                            exit(0);
                        }
                        // Azzeriamo la struttura dati
                        memset(dev_users, 0, sizeof(dev_users));
                        num_users=0;
                        return;
                    }

                    // Se il device con cui stiamo parlando e' offline si stampa un solo asterisco, altrimenti due
                    if(!online){
                        printf("%s *\n", sent);
                    } else{
                        printf("%s **\n", sent);
                    }

                    strcat(sent, " **");
                    fflush(fptr);   
                    // Stampiamo il messaggio nella chat
                    fprintf(fptr, "%s\n", sent);
                    fflush(fptr);
                }

                else if(i==listener){
                    
                    // Il listener e' pronto quando un nuovo componente nella chat si vuole collegare a
                    // tutti i partecipanti della stessa. (For iniziale della funzione (if(grp)))
                    int addrlen = sizeof(dev_addr);
                    new_sd = accept(listener, (struct sockaddr *)&dev_addr, (socklen_t*)&addrlen);

                    // In buffer riceviamo le informazioni del nuovo partecipante
                    recv(new_sd, buffer, sizeof(buffer), 0);

                    if(!strcmp(buffer, CMD_CHAT)){
                        send_dev(new_sd, NO);
                        close(new_sd);
                        break;
                    }

                    sockets[num_sd++] = new_sd;
                    char* estrai = buffer;
                    char scorri[1024];
                    int ch = 0;

                    // Le estraiamo
                    sscanf(estrai, "%s", scorri);
                    strcpy(dev_users[num_users].usr, scorri);
                    printf("%s", entry_msg(scorri));
                    fflush(fptr);   
                    fprintf(fptr, "%s", entry_msg(scorri));
                    fflush(fptr);
                    ch = strlen(scorri)+1;
                    estrai += ch;
                    strcat(users, scorri);
                    strcat(users, "\n");
                    sscanf(estrai, "%s", scorri);
                    // Salviamo nella struttura dati il tutto
                    dev_users[num_users].port = atoi(scorri);
                    dev_users[num_users].sd = new_sd;
                    num_users++;
                    tot_users++;

                    FD_SET(new_sd, &master_chat);                 // Aggiungo il nuovo socket al master
                    fdmax = (new_sd > fdmax) ? new_sd : fdmax;    // Aggiorno fdmax
                   
                }
                else if(i==srv_sd){

                    // Il server e' uscito.
                    printf("ATTENZIONE ! Il server e' andato offline.\n");
                    printf("--> Appena si uscira' dalla chat l'esecuzione terminera'\n");
                    // Si mette a true la variabile globale
                    server_out=true;
                    FD_CLR(i, &master_chat);
                    break;
                }
                else {
                    
                    ret = recv(i, coming, sizeof(coming), 0);
                    
                    if(ret > 0){
                        fflush(stdin);
                        fflush(stdout);
                        if(!strcmp(coming, SHARE)){
                            // E' arrivato il comando di share
                            send_dev(i, RFD);
                            // Riceviamo il nome del file
                            recv(i, filename, sizeof(filename), 0);
                            printf("\n%s sta condividendo un file !\n\n", dev_usr);
                            strcpy(percorso, username);
                            strcat(percorso, "/");
                            strcat(percorso, filename);
                            send_dev(i, RFD);
                            // Riceviamo il contenuto del file
                            recv(i, buffer, sizeof(buffer), 0);
                            fpptr = fopen(percorso, "w");
                            fflush(fpptr);
                            // Stampiamo il contenuto del file nel filename
                            fprintf(fpptr, "%s", buffer);
                            fflush(fpptr);
                            // Stampiamo a video il suo contenuto
                            printf("\nContenuto di %s:\n%s\n", filename, buffer);
                            fflush(fptr); 
                            // Stampiamo il messaggio di sharing nella chat
                            fprintf(fptr, "%s", share_msg(dev_usr));
                            fflush(fptr);
                            break;
                        }
                        else if(!strcmp(coming, EXIT)){
                            // Messaggio di uscita
                            FD_CLR(i, &master_chat);
                            for(j=0; j < num_users; j++){
                                // Tramite la struttura dati ricaviamo l'sd e quindi anche l'username del Device uscito
                                if(dev_users[j].sd == i){
                                    printf("ATTENZIONE ! %s e' uscito dalla chat.\n", dev_users[j].usr);
                                    dev_users[j].sd = 0;
                                }
                            }
                            tot_users--;
                            // A questo punto dobbiamo sistemare la lista degli users da mandare al Server
                            memset(users, 0, sizeof(users));
                            strcat(users, username);
                            strcat(users, "\n");
                            for(y=0; y < num_users; y++){
                                if(dev_users[y].sd){
                                    strcat(users, dev_users[y].usr);
                                    strcat(users, "\n");
                                }
                            }
                            // Se sono usciti tutti, il componente inizia una chat offline con quello che lo ha invitato
                            // Altrimenti continuano a chattare quegli altri
                            if(!tot_users){
                                if(server_out){
                                    printf("ATTENZIONE ! Il server e' offline, l'esecuzione viene interrotta.\n");
                                    exit(0);
                                }
                                printf("I messaggi verranno comunque inoltrati a %s !\n", dev_usr);
                                online = false;
                                offline_chat(srv_sd, dev_usr, fptr);
                                return;
                            }
                        }
                        // La stessa cosa in precedenza solo con il comando GRPEXIT
                        else if(!strcmp(coming, GRPEXIT)){
                            for(j=0; j < num_users; j++){
                                if(dev_users[j].sd == i){
                                    printf("ATTENZIONE ! %s e' uscito dalla chat.\n", dev_users[j].usr);
                                    FD_CLR(i, &master_chat);
                                    tot_users--;
                                    dev_users[j].sd = 0;
                                    memset(users, 0, sizeof(users));
                                    strcat(users, username);
                                    strcat(users, "\n");
                                    for(y=0; y < num_users; y++){
                                        if(dev_users[y].sd){
                                            strcat(users, dev_users[y].usr);
                                            strcat(users, "\n");
                                        }
                                    }
                                    close(i);
                                    if(!tot_users){
                                        if(server_out){
                                            printf("ATTENZIONE ! Il server e' offline, l'esecuzione viene interrotta.\n");
                                            exit(0);
                                        }
                                        printf("ATTENZIONE ! Tutti i componenti della chat sono usciti.\n");
                                        printf("I messaggi verranno inoltrati come pendenti a %s !\n", dev_usr);
                                        online = false;
                                        offline_chat(srv_sd, dev_usr, fptr);
                                        return;
                                    }
                                    //printf("tot_users: %d.\n", tot_users);
                                    break;
                                }
                            }
                        }
                        else{
                            // Stampiamo a video il messaggio che arriva
                            printf("%s\n", coming);
                            if(strcmp(sent, EXIT)){
                                fflush(fptr);
                                fprintf(fptr, "%s\n", coming);
                                fflush(fptr);
                            }
                        }
                        fflush(stdout);
                        fflush(stdin);
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
    //char lista[1024];
    struct sockaddr_in dev_addr;
    int dev_port;
    int dev_sd;
    int ret;

    // Inseriamo l'utente con cui si inizia la chat
    scanf("%s", dev_usr);

    if(!strcmp(dev_usr, username)){
        printf("\nATTENZIONE ! Si prega di non inserire il proprio username.\n");
        return;
    }

    // Mandiamo il comando
    send_srv(sd, CMD_CHAT);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            break;
        }
    }

    send_srv(sd, username);

    // Creo username/rubrica.txt

    strcat(percorso, "/");  
    strcat(percorso, "rubrica.txt");    

    // Creo/apro in lettura la rubrica
    fptr = fopen(percorso, "a+");
    fflush(fptr);
    fclose(fptr);
    
    // printf("Si prega di inserire l'username con cui si vuole aprire una chat:\n\n-> ");

    // Inviamolo al Server
    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            break;
        }
    }

    send_srv(sd, dev_usr);
    
    // Riceveremo una risposta
    recv(sd, buffer, sizeof(buffer), 0);

    if(!strcmp(buffer, NO)){
        printf("\nATTENZIONE ! Username inesistente.\n");
        return;
    }
    
    // Se e' offline diamo comunque la possibilità al Device di inviare il messaggio
    if(!strcmp(buffer, OFFLINE)){
        printf("Username offline, il messaggio verra' comunque inviato:\n\n-> ");
        strcpy(buffer, msg());

        // Inviamo il messaggio al Server che lo inserira' nei pendenti
        send_srv(sd, buffer);
        strcpy(percorso, username);
        strcat(percorso, "/chat");
        mkdir(percorso, 0700);
        strcat(percorso, "/");
        strcat(percorso, dev_usr);
        strcat(percorso, ".txt");
        fptr = fopen(percorso, "a");
        fflush(fptr);
        // Stampiamo comunque nella nostra chat il messaggio
        fprintf(fptr, "%s **\n", buffer);
        fflush(fptr);
        fclose(fptr);
        return;
    }

    // A questo punto il Server ci ha confermato che il device contattato e' online
    // Il Server ci ha inviati la porta del Device
    dev_port = atoi(buffer);
    // In questo pezzo di codice salviamo l'username e la porta in rubrica
    fptr = fopen(percorso, "a+");
    fflush(fptr);
    if(!check_word(fptr, dev_usr)){
         fprintf(fptr, "%s ", dev_usr);
         fprintf(fptr, "%s\n", buffer);
    }
    fclose(fptr);
   
    // Se arriviamo qui vuol dire che stiamo iniziando una chat con il Device (che e' ONLINE)
    
    // Recuperiamo i messaggi pendenti
    pendent_before_chat(sd, dev_usr);

    // Recuperiamo il socket descriptor
    dev_sd = dev_connect(&dev_addr, dev_port);

    // Provo a connettermi al dispositivo
    ret = connect(dev_sd, (struct sockaddr*)&dev_addr, sizeof(dev_addr));

    if(ret < 0){
        // Dispositivo offline
        printf("ATTENZIONE ! Dispositivo offline !\n");
        return;
    }

    // Inviamo al Device il comando di CHAT
    send_dev(dev_sd, CMD_CHAT);

    while(1){
        recv(dev_sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            break;
        } else if(!strcmp(buffer, NO)){
            printf("\nATTENZIONE ! Device impegnato in una chat !\n");
            return;
        }
    }
    // Inviamo al Device il nostro username
    send_dev(dev_sd, username);

    srv_sd = sd;

    printf("In attesa che %s accetti la chat...\n", dev_usr);

    while(1){

        recv(dev_sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            // Inviamo anche la porta al Device
            sprintf(buffer, "%d", my_port);
            send(dev_sd, &buffer, sizeof(&buffer), 0);
            // Bisogna controllare se c'e' gia'
            strcpy(dev_users[num_users].usr, dev_usr);
            dev_users[num_users].port = dev_port;
            dev_users[num_users].sd = dev_sd;
            num_users++;
            // Iniziamo la chat, false -> non di gruppo
            chat(dev_sd, dev_usr, false);
            break;
        }

        // Il Device a sua volta puo' rifiutare la chat, in questo caso iniziamo una chat offline
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
            printf("\nLa chat e' stata aperta, i messaggi verranno comunque inviati.\n");
            printf("\nPer uscire digitare '\\q'");
            offline_chat(srv_sd, dev_usr, fptr);
            break;
        }
    }


}

// Questa funzione si occupa del logout del Device
void out_config(int sd){

    char buffer[1024];

    // Invio comando al Server
    send_srv(sd, CMD_OFF);

    // Bisogna utilizzare RFD
    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        // Inviamo al Server l'username
        send_srv(sd, username);
        break;
    }

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        // Inviamo al server la porta
        send(sd, &my_port, sizeof(&my_port), 0);
        break;
    }

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        // Inviamo al server il timestamp
        send(sd, timestamp, sizeof(timestamp), 0);
        break;
    }

    strcat(percorso, "/rubrica.txt");

}

// Questa funzione si occupa del comando 'hanging' digitato dal Device
void hanging_config(int sd){

    char buffer[1024];

    // Inviamo al server il comando
    send_srv(sd, CMD_HANGING);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            // Inviamo l'username
            send_srv(sd, username);
            break;
        }
    }
    
    // Riceviamo il messaggio e lo stampiamo a video
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
    FILE* fptr;

    // Inviamo il comando
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
            // Inviamo l'username
            send_srv(sd, dev_usr);
            break;
        }
    }

    // Riceviamo il messaggio
    recv(sd, buffer, sizeof(buffer), 0);
    
    // NO indica che l'username inserito e' sbagliato
    if(!strcmp(buffer, NO)){
        printf("\nNessun messaggio pendente dall'utente selezionato !\n");
        printf("\nDigitare qualsiasi cosa per presa visione.\n");
        scanf("%s", buffer);
        return;
    }   

    // " " indica che non c'e' nessun messaggio pendente dall'utente delezionato
    if(!strcmp(buffer, " ")){
        printf("\nNessun messaggio pendente dall'utente selezionato !\n");
        printf("\nDigitare qualsiasi cosa per presa visione.\n");
        scanf("%s", buffer);
        return;
    }
    
    // Stampiamo il messaggio
    printf("%s", buffer);

    // Aggiorniamo la chat con i messaggi pendenti letti
    strcpy(percorso, username);
    strcat(percorso, "/chat/");
    strcat(percorso, dev_usr);
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
    

    // Se il Server e' offline si esce subito
    if(server_out){
        exit(0);
    }

    //printf("In attesa del Server...\n");

    // Se si sta chattando prima si manda il messaggio di exit 
    if(chatting){
        send_dev(dev_sd, EXIT);
        //printf("Il Device sta terminando ...\n");
    }

    // Se e' online si chiama la funzione di uscita
    if(online){
        out_config(srv_sd);
        printf("\n");
        exit(0);
    }
}

// Handler di escape
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
    FILE* fptr;

    if(argc < 2){
        printf("ATTENZIONE! Inserisci una porta.\n");
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
        scanf("%s", buffer);

        srv_port = atoi(buffer);

        // Configuriamo il socket connesso al server
        srv_sd = srv_config(&srv_addr, srv_port);
        // Connettiamo il Device al Server
        ret = connect(srv_sd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
        if(ret < 0){
            printf("\nATTENZIONE! Server offline sulla porta %d.\n\n(La porta 4242 e' quella di default).\n", srv_port);
            continue;
        }
        break;
    }

    // Azzero i set
    FD_ZERO(&master);
    FD_ZERO(&read_fds); 
    
    // Aggiungo il listener al set master
    // Aggiungo il descrittore della STDIN al set master
    FD_SET(STDIN, &master);
    FD_SET(listener, &master);

    // Il socket maggiore
    fdmax = listener;

    // Prima stampa
    first_print();
    signal(SIGTSTP, handler1);   // CTRL+Z o chiusura terminale


    printf("\n\n---> Digita 'signup' per creare un account.\n\n");
    printf("---> Se hai gia' un account registrato digita 'login'.\n\n");
    printf("---> Per uscire digitare 'out'.\n\n-> ");

    while(1){       
        
        // Si chiede a video un comando di registrazione o login
        scanf("%s", spacenter);

        if(!strcmp(spacenter, "out")){
            exit(0);
        }

        else if(!strcmp(spacenter, "signup") || !strcmp(spacenter,"SIGNUP")){
            printf("\n---> Registrazione account in corso...\n\n");
            // La funzione si occupa di tutta la fase di registrazione comprensiva di richiesta di username e password
            reg_config(srv_sd);
            printf("\n---> L'account e' stato registrato correttamente\n");
            break;

        } else if(!strcmp(spacenter, "login") || !strcmp(spacenter, "LOGIN")){

            // La funzione si occupa di tutta la fase di login
            // E' una booleana: se ritorna true vuol dire che l'utente vuole fare la signup
            if(log_config(srv_sd)){
                    printf("\n---> Registrazione account in corso...\n\n");
                    // La funzione si occupa di tutta la fase di registrazione comprensiva di richiesta di username e password
                    reg_config(srv_sd);
                    printf("\n---> L'account e' stato registrato correttamente\n");
            }
            break;
        }
        else{
            // Comando sbagliato
            printf("\nATTENZIONE ! Comando -%s- non riconosciuto.\n", spacenter);
            printf("\n--> Digita 'signup' per creare un account.\n\n");
            printf("---> Se hai gia' un account registrato digita 'login'.\n\n");
            printf("---> Per uscire digitare 'out'.\n\n-> ");
            continue;
        }
    }

    // Adesso il Device e' online, dobbiamo inviare al Server il timestamp corrente
    online_config(srv_sd, my_port);

    // Il device e' ufficialmente online
    online = true;

    FD_SET(srv_sd, &master);
    fdmax = (srv_sd > fdmax) ? srv_sd : fdmax;

    // Handler per la gestione della disconnessione del device improvvisa
    signal(SIGINT, handler);    // CTRL+C
    signal(SIGILL, handler);
    signal(SIGKILL, handler);
    signal(SIGSTOP, handler);

    printf("\nSalve %s ! Benvenuto nel sistema di chatting.\n", username);

    // Creiamo la cartella dell'username
    mkdir(username, 0700);

    // Creiamo il file di esempio
    strcpy(buffer, username);
    strcat(buffer, "/ex_");
    strcat(buffer, username);
    strcat(buffer, ".txt");
    fptr = fopen(buffer, "w");
    fflush(fptr);
    strcpy(buffer, "Questo e' un file di esempio.\nLista della spesa:\n-Carote\n-Cipolle\n-Mandarini");
    fprintf(fptr, "%s", buffer);
    fclose(fptr);

    memset(dev_users, 0, sizeof(dev_users));
    num_users=0;
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
                    //printf("Gestione comando %s\n", buffer);

                    if(!strcmp(buffer, "hanging")){
                        // Gestione comando hanging
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

                else if(i == listener) {                        // Se quello pronto e' il listener
  
                    addrlen = sizeof(dev_addr);
                    newfd = accept(listener, (struct sockaddr *)&dev_addr, (socklen_t*)&addrlen);
                    FD_SET(newfd, &master);                     // Aggiungo il nuovo socket al master
                    fdmax = (newfd > fdmax) ? newfd : fdmax;    // Aggiorno fdmax
                   
                }

                else if(i == srv_sd){
                    printf("Il server e' OFFLINE, l'esecuzione viene interrotta.\n");
                    exit(0);
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


                        // Gestione registrazione
                        if(!strcmp(command, CMD_CHAT)){

                            send_dev(i, RFD);

                            // Ricevo l'username che mi ha invitato nella chat
                            recv(i, dev_usr, sizeof(dev_usr), 0);

                            printf("%s vuole iniziare una chat con te !\n\n", dev_usr);

                            printf("---> Digita qualsiasi cosa per entrare nella chat.\n");
                            printf("---> Se si vuole rifutare la chat digitare '\\q'.\n\n");

                            scanf("%s", wait);

                            // Posso rifiutare la chat
                            if(!strcmp(wait, EXIT)){
                                send_dev(i, NO);
                                second_print();
                                break;
                            }

                            // Prima di iniziare la chat recupero i messaggi pendenti
                            pendent_before_chat(srv_sd, dev_usr);

                            send_dev(i, RFD);

                            // Ricevo porta del Device
                            recv(i, dev_port, sizeof(dev_port), 0);

                            // Aggiorno la struttura dati
                            strcpy(dev_users[num_users].usr, dev_usr);
                            dev_users[num_users].port = atoi(dev_port);
                            dev_users[num_users].sd = i;
                            //printf("USR: %s\nPORT: %d\n", dev_users[num_users].usr, dev_users[num_users].port);
                            // Incremento gli users
                            num_users++;

                            // Iniziamo la chat con dev_usr, false indica che per ora non e' una chat di gruppo
                            chat(i, dev_usr, false);
                            // Ristampiamo il menu' iniziale
                            second_print();

                        }
                        else if(!strcmp(command, CMD_GRPCHAT)){

                            num_users = 0;

                            send_dev(i, RFD);

                            // Ricevo l'username che mi ha invitato nella chat di gruppo
                            recv(i, dev_usr, sizeof(dev_usr), 0);

                            printf("%s ti ha invitato ad una chat di gruppo !\n\n", dev_usr);

                            printf("---> Digita qualsiasi cosa per entrare nella chat.\n");
                            printf("---> Se si vuole rifutare la chat digitare '\\q'.\n\n");

                            scanf("%s", wait);
                            
                            // Posso rifiutare la chat
                            if(!strcmp(wait, EXIT)){
                                send_dev(i, NO);
                                second_print();
                                break;
                            }

                            send_dev(i, RFD);

                            // Ricevo porta del device
                            recv(i, dev_port, sizeof(dev_port), 0);
                            
                            // Aggiorno la struttura dati
                            strcpy(dev_users[num_users].usr, dev_usr);
                            dev_users[num_users].port = atoi(dev_port);
                            dev_users[num_users].sd = i;
                            //printf("user:%s\nport:%d.\n", chat_user[num_users].usr, chat_user[num_users].port);
                            num_users++;

                            memset(buffer, 0, sizeof(buffer));

                            send_dev(i, RFD);

                            //Ricevo la lista 
                            recv(i, buffer, sizeof(buffer), 0);
                           
                            char* estrai = buffer;
                            char scorri[1024];
                            int ch = 0;

                            memset(scorri, 0, sizeof(scorri));

                            // Questa parte di codice si occupa di estrarre le informazioni dei Device
                            // che fanno parte della chat di gruppo, in modo tale che nella funzione 
                            // chat() si colleghi correttamente a tutti.
                            while(sscanf(estrai, "%s", scorri)==1){
                                if(strcmp(scorri, username)){
                                    strcpy(dev_users[num_users].usr, scorri);
                                    ch = strlen(scorri)+1;
                                    estrai += ch;
                                    sscanf(estrai, "%s", scorri);
                                    dev_users[num_users].port = atoi(scorri);
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

                            // Chiamiamo la funzione chat, true sta per chat di gruppo
                            chat(i, dev_usr, true);

                            // Ristampiamo tutto
                            second_print();

                        }
                    }   
                    break;
                } 
                }
            } 
        }

    }

    return 0;

}
