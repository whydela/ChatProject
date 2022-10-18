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

#define STDIN 0
#define RFD "/RFD\0"
#define CMD_REG "/REG\0"
#define CMD_LOG "/LOGIN\0"
#define CMD_TMS "/TIMESTAMP\0"
#define CMD_CHAT "/CHAT\0"
#define YES "/YES\0"
#define NO "/NO\0"

char buffer[1024];

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
    printf("SERVER AVVIATO\n");
    for(i=0; i < 190; i++){
        printf("*");
    }
    printf("\n\n\nSi prega di inserire un comando:\n");
    printf("\n- list: Mostra gli utenti online.\n");
    printf("- esc:  Chiude il server.\n");
    printf("\n(Digitare help per i dettagli dei comandi)\n\n");
    

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
        perror("Errore in fase di bind: \n");
        exit(-1); 
    }

    return sd;

}

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

char* filetobuffer(FILE* fptr, char stringa[1024]){

    char scorre[1024];

    memset(buffer, 0, sizeof(*buffer));         // Pulizia della variabile globale
    while(fscanf(fptr, "%s", scorre)==1){
        if(!strcmp(scorre, stringa)){
            continue;
        }
        strcat(scorre, "\n");
        strcat(buffer, scorre);
    }

    return buffer;

}

void send_dv(int sd, char* cmd){
    // In questo momento sono connesso al server 
    int ret;
    
    //printf("Mando il segnale %s\n", cmd);
    ret = send(sd, cmd, strlen(cmd)+1, 0);

    if(ret < 0){
        perror("Errore in fase di invio segnale: \n");
        exit(1);
    }
    
    printf("Segnale %s inviato\n", cmd);
    
}

void dev_reg(int sd){

    char username[1024];
    char password[1024];

    while(1){

        FILE* fptr;
        // Ricevo username
        recv(sd, username, sizeof(username), 0);

        printf("Ricevuto %s\n",username);
        // creo/apro un file contenente tutti gli username registrati
        

        // Questa funzione ricerca una stringa in un file, 
        // Restituisce true se la trova, altrimenti false
        // Prende in ingresso il puntatore del file e la stringa ricercata
        fptr = fopen("srv/usr_all.txt", "r");

        if(fptr && check_word(fptr, username)){
            // Dico al device che l'username non va bene
            // YES sta per: c'e' nella lista degli username registrati
            send_dv(sd, YES);
            // Si richiede
            continue;
        }

        // Dico al device che l'username va bene
        // NO sta per: non c'e' nella lista degli username registrati
        send_dv(sd, NO);

        if (fptr) {
            fclose(fptr);
        } 

        fptr = fopen("srv/usr_all.txt", "a");

        if(!fptr){
            printf("Errore nell'apertura del file\n");
        }

        fprintf(fptr, "%s\n", username);

        // Facciamo in modo che i permessi siano attivi solo per l'owner del file
        // Come se il server avesse delle informazioni non accessibili dai device
        chmod("srv/usr_all.txt", S_IRWXU);

        if(fptr){
            fclose(fptr);
        }

        // Adesso gestiamo la password
        recv(sd, password, sizeof(password), 0);
        printf("Ricevuto %s\n",password);

        // Apro/creo un file contenente tutti gli username registrati con la password affiancata
        fptr = fopen("srv/usr_psw.txt", "a+");
        fprintf(fptr, "%s\n", password);

        // Facciamo in modo che i permessi siano attivi solo per l'owner del file
        // Come se il server avesse delle informazioni non accessibili dai device
        chmod("srv/usr_psw.txt", S_IRWXU);
        if(fptr){
            fclose(fptr);
        }

        break;
    }

    printf("%s registrato nel sistema\n", username);

}

bool dev_log(int sd){

    char username[1024];
    char password[1024];

    FILE* fptr;
    
    while(1){
        printf("Gestione username\n");
        // Ricevo username
        recv(sd, username, sizeof(username), 0);

        printf("Ricevuto %s\n",username);

        if(!strcmp(username, "signup")){
            printf("Si vuole registrare\n");    
            return true;
        }

        // Creo/apro un file contenente tutti gli username registrati
        // Lo apro in scrittura perche' nel caso in cui ci sia un tentativo di login come primo comando in assoluto,
        // aprire il file in sola lettura non implica la sua creazione, ciò provocherebbe una crisi di consistenza 
        fptr = fopen("srv/usr_all.txt", "r");

        if(!fptr){
            printf("Errore nell'apertura del file\n");
            send_dv(sd, NO);
            continue;
        }

        // Questa funzione ricerca una stringa in un file, 
        // Restituisce true se la trova, altrimenti false
        // Prende in ingresso il puntatore del file e la stringa ricercata
        if(check_word(fptr, username)){
            // Dico al device che l'username non va bene
            // YES sta per: c'e' nella lista degli username registrati
            send_dv(sd, YES);
        } else{
            // Si richiede
            // Dico al device che l'username va bene
            // NO sta per: non c'e' nella lista degli username registrati
            send_dv(sd, NO);
            continue;
        }

        if(fptr){
            fclose(fptr);
        }

        break;
    }

    // Adesso gestiamo la password
    while(1){
            printf("Gestione password\n");
            recv(sd, password, sizeof(password), 0);
            printf("Ricevuto %s\n", password);

            if(!strcmp(password, "signup")){
                printf("Si vuole registrare\n");    
                return true;
            }

            // Apro/creo un file contenente tutti gli username registrati con la password affiancata
            fptr = fopen("srv/usr_psw.txt", "r");

            // Questa funzione ricerca una stringa in un file, 
            // Restituisce true se la trova, altrimenti false
            // Prende in ingresso il puntatore del file e la stringa ricercata

            if(check_word(fptr, password)){
                // Dico al device che l'username non va bene
                // YES sta per: c'e' nella lista degli username registrati
                send_dv(sd, YES);
            } else{
                send_dv(sd, NO);
                // Si richiede
                continue;
            }

            if(fptr){
                fclose(fptr);
            }
            break;        

    }

    printf("%s loggato nel sistema\n", username);

    return false;

}

void dev_online(int sd){

    char buffer[1024];
    char timestamp[1024];
    char username[1024];
    int port;
    time_t rawtime;
    struct tm * timeinfo;
    bool online;
    FILE* fptr;

    // Ricevo username
    recv(sd, username, sizeof(username), 0);
    printf("Ricevuto %s\n", username);

    time(&rawtime);

    // Converto l'ora
    timeinfo = localtime(&rawtime);

    // Creo il timestamp
    sprintf(timestamp, "%d-%d-%d|%d:%d:%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,
    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    fptr = fopen("srv/usr_online.txt", "a+");
    online = check_word(fptr, username);

    if(!online){
        send_dv(sd, NO);
        recv(sd, &port, sizeof(port), 0);
        //fptr = fopen("srv/usr_online.txt", "a");
        fprintf(fptr, "%s\n", username);
        fprintf(fptr, "%d\n", port);
        fprintf(fptr, "%s\n", timestamp);
        // Indica che l'utente non ha ancora effettuato il log out
        fprintf(fptr, "%d\n", 0);
    }
    else{
        send_dv(sd, YES);
    }

    chmod("srv/usr_online.txt", S_IRWXU);

    if(fptr){
        fclose(fptr);
    }

    printf("Ricevuto %s\n", timestamp);

    sprintf(buffer, "%s*%s*%d", username, timestamp, port);

    // Creo/apro un file contenente tutti gli username registrati
    // Lo apro in scrittura perche' nel caso in cui ci sia un tentativo di login come primo comando in assoluto,
    // aprire il file in sola lettura non implica la sua creazione, ciò provocherebbe una crisi di consistenza
    fptr = fopen("srv/usr_log.txt", "a+");

    if(!online){
        fprintf(fptr, "%s\n", buffer);
    }

    chmod("srv/usr_log.txt", S_IRWXU);

    if(fptr){
        fclose(fptr);
    }

}

void dev_chat(int sd){

    // Preparo la rubrica
    char buffer[1024];
    char username[1024];
    FILE* fptr;

    fptr = fopen("srv/usr_all.txt", "r");
    
    recv(sd, username, sizeof(username), 0);

    strcpy(buffer, filetobuffer(fptr, username));

    send_dv(sd, buffer);

}

void srv_list(){

    char buffer[1024];

    FILE* fptr = fopen("srv/usr_log.txt", "r");

    if(!fptr){
        printf("Nessun utente e' attualmente online\n");
        return;
    }

    printf("\nLista degli utenti online:\n\n");
    
    while(fscanf(fptr, "%s", buffer)==1){
        printf("- %s\n", buffer);
    }

    fclose(fptr);

}

void srv_help(){

    printf("\n--> Il comando 'list' permette di vedere la lista degli utenti online");
    printf(" nel formato username*timestamp*port:\n\n");
    
    printf("\t- 'username'  -> rappresenta l'id dell'utente online.\n");
    printf("\t- 'timestamp' -> rappresenta l'istante in cui l'utente si e'");
    printf(" loggato nel sistema nel formato dd-mm-yyyy|hh-mm-ss.\n");
    printf("\t- 'port'      -> rappresenta la porta a cui si e' collegato l'username corrente.\n");

    printf("\n--> Il comando 'esc' permette di terminare il Server.\n\n");
    printf("\t- La disconnessione del Server non implica un'interruzione di");
    printf(" servizio per i Device che stanno comunicando tra loro.\n");
    printf("\t- Un Device non puo' loggarsi con il Server spento.\n\n");
    
}



int main(int argc, char *argv[]) {
    // Dichiarazioni Variabili

    int port;                                       // Porta a cui e' associato il Server
    int ret;                                        // Variabile di controllo
    fd_set master;                                  // Set principale gestito dal programmatore con le macro 
    fd_set read_fds;                                // Set di lettura gestito dalla select 
    int fdmax;                                      // Numero max di descrittori
    struct sockaddr_in my_addr;                     // Indirizzo Server
    struct sockaddr_in cl_addr;                     // Indirizzo Client 
    int listener;                                   // Socket di ascolto
    int newfd;                                      // Socket di comunicazione
    char command[1024];
    char buffer[1024];
    int i;
    int addrlen;

    
    // Questa funzione si occupa della prima stampa a video
    first_print();

    // Si inizializza la porta del Server, atoi converte da char ad intero
    port = (argc > 1) ? atoi(argv[1]) : 4242;

    // Azzero i set
    FD_ZERO(&master);
    FD_ZERO(&read_fds); 

    // Questa funzione si occupa della configurazione del socket di ascolto:
    // Il primo argomento riguarda un puntatore alla struttura dell'indirizzo del Server (struct sockaddr_in*)
    // Il secondo argomento riguarda la porta a cui si deve connettere (int)
    // La funzione restituisce il descrittore del socket di ascolto, chiamato listener
    listener = ip_config(&my_addr, port);

    // Il server crea una coda di 10 ascolti 
    ret = listen(listener, 10);

    if(ret < 0){
        perror("Errore nella listen: ");
        exit(-1);
    }

    // Creo la cartella del Server
    mkdir("srv", 0700);

    // Aggiungo il listener al set master
    FD_SET(listener, &master);
    // Aggiungo il descrittore della STDIN al set master
    FD_SET(STDIN, &master);

    // Il socket maggiore sara' il listener
    fdmax = listener;

    // Ciclo principale
    while(1){

        read_fds = master;
        printf("Parte il ciclo e chiamo la select\n\n");
        
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        for(i=0; i<=fdmax; i++) {
            // Cerco quelli pronti
            if(FD_ISSET(i, &read_fds)) {                    // Trovato socket pronto

                if(!i){                                     // Quello pronto riguarda la stdin
                    scanf("%s", buffer);
                    if(!strcmp(buffer, "list")){
                        // Gestione comando list
                        srv_list();
                    }

                    else if(!strcmp(buffer, "esc")){
                        // Gestione comando esc
                        // Per ora esco
                        exit(0);
                    }

                    else if(!strcmp(buffer, "help")){
                        // Gestione comando help
                        srv_help();
                    }

                    else{
                        printf("\nATTENZIONE ! Comando -%s- inesistente.\n", buffer);
                        printf("\nSi prega di inserire un comando:\n");
                        printf("\n- list: Mostra gli utenti online.\n");
                        printf("- esc:  Chiude il server.\n");
                        printf("\n(Digitare help per i dettagli dei comandi)\n\n");
                    }
                }

                else if(i == listener) {                    // Se quello pronto e' il listener
                    
                    addrlen = sizeof(cl_addr);

                    newfd = accept(listener, (struct sockaddr *)&cl_addr, (socklen_t*)&addrlen);

                    printf("Ho accettato la connessione sul listener, aggiungo nuovo socket al SET\n");

                    FD_SET(newfd, &master);                     // Aggiungo il nuovo socket al master
                    fdmax = (newfd > fdmax) ? newfd : fdmax;    // Aggiorno fdmax

                   
                } else{                                     // Se il socket pronto e' il comunicatore

                while(1){
                    ret = recv(i, command, sizeof(command), 0);   

                    if(!ret){                               // Socket i e' stato chiuso, (Device Offline ?) 
                        printf("Socket chiuso\n");
                        FD_CLR(i, &master);                 // Lo tolgo dal master 
                        close(i);                           // Lo chiudo
                        printf("Chiudo il socket %d e lo tolgo dal set\n", i);

                    } else if(ret > 0){                     // Qui arriva il SEGNALE /XXX

                        //printf("Il comunicatore (socket %d) e' pronto\n", i);
                        printf("E' arrivato il comando %s\n", command);

                        // Gestione registrazione
                        if(!strcmp(command,CMD_REG)){
                            // La funzione si occupa della corretta registrazione del device
                            // Prende in ingresso il socket descriptor
                            printf("Gestione registrazione\n");
                            dev_reg(i);
                        }

                        // Gestione login
                        else if(!strcmp(command, CMD_LOG)){
                            printf("Gestione login \n");
                            // La funzione si occupa del login del device
                            // Prende in ingresso il socket descriptor
                            if(dev_log(i)){
                                continue;
                            }
                        }

                        // Gestione timestamp
                        else if(!strcmp(command, CMD_TMS)){
                            printf("Gestione timestamp\n");
                            dev_online(i);
                        }

                        // Gestione chat
                        else if(!strcmp(command, CMD_CHAT)){
                            printf("Gestione chat\n");
                            dev_chat(i);
                        }
                    } else{
                        perror("Errore nella reiceve: ");
                    }
                    break;
                } 
                }     
            }
        }
    }
    return 0;
}
