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
#define RFD "RFD\0"
#define CMD_REG "/REG\0"
#define CMD_LOG "/LOGIN\0"
#define CMD_TMS "/TIMESTAMP\0"
#define CMD_CHAT "/CHAT\0"
#define CMD_OFF "/OFF"
#define YES "/YES\0"
#define NO "/NO\0"
#define OFFLINE "/OFFLINE\0"

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

int dev_config(struct sockaddr_in* dev_addr, int dev_port){

    int sd;
    
    sd = socket(AF_INET, SOCK_STREAM, 0);   // Creazione socket
    memset(dev_addr, 0, sizeof(*dev_addr));         // Pulizia

    (*dev_addr).sin_family = AF_INET;          // Address family
    (*dev_addr).sin_port = htons(dev_port);

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

void change_log(char username[1024]){
    
    FILE* fptr, *fpptr;
    char buffer[1024];
    char timestamp[1024];
    int i = 0;
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    fptr = fopen("srv/usr_log1.txt", "a");
    fpptr = fopen("srv/usr_log.txt", "r");
    fflush(fptr);
    fflush(fpptr);

    memset(timestamp, 0, sizeof(timestamp));

    sprintf(timestamp, "%d-%d-%d|%d:%d:%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,
    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    // Questo passaggio permette di aggiornare il timestamp di logout degli utenti

    while(fscanf(fpptr, "%s", buffer)==1){
        if(!strcmp(buffer, username)){
            while(i < 4){
                if(i==3 && !strcmp(buffer, "0")){
                    fprintf(fptr, "%s\n", timestamp);
                    break;
                } else{
                    fprintf(fptr, "%s\n", buffer);
                }
                fscanf(fpptr, "%s", buffer);
                i++;
            }
        }
        else{
            fprintf(fptr, "%s\n", buffer);
        }
    }

    remove("srv/usr_log.txt");
    rename("srv/usr_log1.txt", "srv/usr_log.txt");

    fclose(fptr);
    fclose(fpptr);

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
    FILE* fptr, *fpptr, *fppptr;

    // Ricevo username
    send_dv(sd, RFD);
    recv(sd, username, sizeof(username), 0);
    printf("Ricevuto %s\n", username);

    time(&rawtime);

    // Converto l'ora
    timeinfo = localtime(&rawtime);

    // Creo il timestamp

    fptr = fopen("srv/usr_log.txt", "a+");
    //online = check_word(fptr, username);

    send_dv(sd, RFD);
    sprintf(timestamp, "%d-%d-%d|%d:%d:%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,
    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    recv(sd, &port, sizeof(port), 0);

    // Se l'username non e' presente nel file si scrivono tutti i valori del caso
    if(!check_word(fptr, username)){  
        fprintf(fptr, "%s\n", username);
        fprintf(fptr, "%d\n", port);
        fprintf(fptr, "%s\n", timestamp);
        // Indica che l'utente non ha ancora effettuato il log out
        fprintf(fptr, "%d\n", 0);

    } 
    // Altrimenti si sovrascrive il timestamp di login mettendo quello corrente, 
    // e quello di log out mettendo 0
    else{
        fppptr = fopen("srv/usr_log.txt", "r");
        fpptr = fopen("srv/usr_log1.txt", "a+");
        fflush(fpptr);
        fflush(fppptr);
        while(fscanf(fppptr, "%s", buffer)==1){
            printf("%s\n", buffer);
            if(!strcmp(buffer, username)){
                fprintf(fpptr, "%s\n", username);
                fscanf(fppptr, "%s", buffer);
                fprintf(fpptr, "%d\n", port);
                fscanf(fppptr, "%s", buffer);
                fprintf(fpptr, "%s\n", timestamp);
                fscanf(fppptr, "%s", buffer);
                fprintf(fpptr, "%d\n", 0);
                } 
                else{
                    fprintf(fpptr, "%s\n", buffer);
                }
        }
        fclose(fpptr);
        fclose(fppptr);
        remove("srv/usr_log.txt");
        rename("srv/usr_log1.txt", "srv/usr_log.txt");
    }

    chmod("srv/usr_log.txt", S_IRWXU);

    if(fptr){
        fclose(fptr);
    }

    printf("Ricevuto %s\n", timestamp);


    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%s\n%s\n%d", username, timestamp, port);

    // Creo/apro un file contenente tutti gli username registrati
    // Lo apro in scrittura perche' nel caso in cui ci sia un tentativo di login come primo comando in assoluto,
    // aprire il file in sola lettura non implica la sua creazione, ciò provocherebbe una crisi di consistenza
    fptr = fopen("srv/usr_online.txt", "a+");
    fflush(fptr);
    if(!check_word(fptr, username)){
        fprintf(fptr, "%s\n", buffer);
    }

    chmod("srv/usr_online.txt", S_IRWXU);

    if(fptr){
        fclose(fptr);
    }


}

void crea_lista(int sd, char username[1024]){
    
    //char buffer[1024];
    char scorre[1024];
    char lista[1024];

    FILE* fptr, *fpptr;

    fptr = fopen("srv/usr_all.txt", "r");
    fpptr = fopen("srv/usr_online.txt", "r");
    
    // Ready for data
    send_dv(sd, RFD);

    // Si riceve l'username che vuole iniziare la chat
    recv(sd, username, sizeof(username), 0);

    memset(lista, 0, sizeof(lista));        // Pulizia

    while(fscanf(fptr, "%s", scorre)==1){
        // Se si trova lo stesso username si ignora
        if(!strcmp(scorre, username)){
            continue;
        }
        printf("%s\n", scorre);
        // Se e' online
        if(check_word(fpptr, scorre)){
           strcat(scorre, ", ONLINE.");
        }
        // Se e' offline
        else{
            strcat(scorre, ", OFFLINE.");
        }
        strcat(scorre, "\n");
        strcat(lista, scorre);
    }

    // Inviamo al device la lista
    send_dv(sd, lista);

}

bool first_chat(int sd, char username[1024]){

    FILE* fptr;
    struct sockaddr_in dev_addr;
    int dev_sd, ret;
    char dev_port[1024];
    char scorre[1024];

    // Estraiamo la porta        
    fptr = fopen("srv/usr_log.txt", "r");
    fflush(fptr);

    while(fscanf(fptr, "%s", scorre)==1){
    // Si estrae la porta dell'username richiesto per una chat
        if(!strcmp(scorre, username)){
            fscanf(fptr, "%s", dev_port);
            break;
        }
    }

    // Il Server prova a connettersi al device
    dev_sd = dev_config(&dev_addr, atoi(dev_port));

    // Connettiamo il Device al Server
    ret = connect(dev_sd, (struct sockaddr*)&dev_addr, sizeof(dev_addr));

    if(ret < 0){
        change_log(username);
        send_dv(sd, OFFLINE);
        return false;
    }

    send_dv(sd, dev_port);
    return true;

}

void dev_chat(int sd){

    // Preparo la lista degli utenti online e offline
    char username[1024];
    crea_lista(sd, username);
    
    //char buffer[1024];
    //char scorre[1024];
    char percorso[1024];
    char dev_usr[1024];
    //char dev_port[1024];

    FILE* fptr, * fpptr;
    // percorso -> srv/
    strcpy(percorso, "srv/");
    

    while(1){

        // Si riceve l'username richiesto per una chat
        recv(sd, dev_usr, sizeof(dev_usr), 0);

        fptr = fopen("srv/usr_log.txt", "r");
        fflush(fptr);

        // Se non esiste si invia al device NO
        if(!check_word(fptr, dev_usr)){
            send_dv(sd, NO);
            // E si ricevera' un ulteriore username da controllare
            continue;
        }

        fclose(fptr);
        
        send_dv(sd, YES);
        
        recv(sd, buffer, sizeof(buffer), 0);

        // Se si riceve YES dal device, vuol dire che e' la prima chat
        if(!strcmp(buffer, YES)){

            if(first_chat(sd, dev_usr)){
                printf("ESCO\n");
                return;
            }

            // Adesso il server deve inoltrare il messaggio al mittente
            recv(sd, buffer, sizeof(buffer), 0);
            
            // Si invia il messaggio in una directory contenente i messaggi pendenti
            // percorso -> srv/dev_usr
            strcat(percorso, dev_usr);
            // Creiamo la cartella srv/dev_usr
            mkdir(percorso, 0700);

            // Creiamo la cartella dei messaggi pendenti srv/dev_usr/pendent
            strcat(percorso, "/pendent");
            mkdir(percorso, 0700);

            strcat(percorso, "/");
            strcat(percorso, username);
            strcat(percorso, ".txt");

            // Creiamo il file srv/dev_usr/pendent/username.txt
            //printf("Il percorso e' %s\n", percorso);
            fpptr = fopen(percorso, "a");
            fflush(fpptr);
            fprintf(fpptr, "%s\n", buffer);
            fflush(fpptr);
            return;

        }
        
        break;

    }

    

}

void dev_out(int sd){

    FILE* fptr, *fpptr;
    char username[1024];
    char timestamp[1024];
    char buffer[1024];
    char usr_port[1024];
    int port;  

    send_dv(sd, RFD);
    recv(sd, username, sizeof(username), 0);

    send_dv(sd, RFD);
    recv(sd, &port, sizeof(&port), 0);

    send_dv(sd, RFD);
    recv(sd, timestamp, sizeof(timestamp), 0);

    printf("%s sta andando OFFLINE !\n", username);

    // Tolgo dalla lista degli utenti online l'utente selezionato
    fptr = fopen("srv/usr_online1.txt", "a");
    fpptr = fopen("srv/usr_online.txt", "r");
    fflush(fptr);
    fflush(fpptr);

    sprintf(usr_port, "%d", port);

    while(fscanf(fpptr,"%s", buffer)==1){
        if(!strcmp(buffer, username) || !strcmp(buffer, usr_port) || !strcmp(buffer, timestamp)){
            continue;
        }
        printf("%s\n", buffer);
        fprintf(fptr, "%s\n", buffer);
    }

    remove("srv/usr_online.txt");
    rename("srv/usr_online1.txt", "srv/usr_online.txt");

    fclose(fptr);
    fclose(fpptr);

    // Log sistemato

    // Metto il timestamp di log out uguale a quello corrente
    change_log(username);


}

void srv_list(){

    char buffer[1024];
    char stringa[1024];

    FILE* fptr = fopen("srv/usr_online.txt", "r");

    if(!fptr){
        printf("Nessun utente e' attualmente online\n");
        return;
    }

    printf("\nLista degli utenti online:\n\n");
    
    while(fscanf(fptr, "%s", buffer)==1){
        int i = 0;
        strcat(buffer, "*");
        while(i < 2){
            fscanf(fptr, "%s", stringa);
            strcat(buffer, stringa);
            if(i != 1){
                strcat(buffer, "*");
            }
            i++;
        }
        printf("%s\n", buffer);
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
    struct sockaddr_in cl_addr;                     // Indirizzo Device 
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

                    if(!ret){                               // Socket i e' stato chiuso, Device offline
                        printf("Socket chiuso\n");
                        FD_CLR(i, &master);                 // Lo tolgo dal master 
                        close(i);                           // Lo chiudo
                        printf("Chiudo il socket %d e lo tolgo dal set\n", i);

                    } 
                    else if(ret > 0){                     // Qui arriva il SEGNALE /XXX

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

                        else if(!strcmp(command, CMD_OFF)){
                            printf("Gestione out\n");
                            dev_out(i);
                        }
                    } 
                    else{
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
