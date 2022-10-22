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

#define RFD "RFD\0"
#define CMD_REG "/REG\0"
#define CMD_LOG "/LOGIN\0"
#define CMD_TMS "/TIMESTAMP\0"
#define CMD_CHAT "/CHAT\0"
#define CMD_OFF "/OFF"
#define YES "/YES\0"
#define NO "/NO\0"
#define STDIN 0

char username[1024];                    // username del device
char password[1024];                    // password del device
char timestamp[1024];                   // username*timestamp*porta del device
char percorso[1024];                    // percorso: username/file.txt
char rubrica[1024];                     // rubrica

int my_port;                            // porta del device


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

int ip_config(struct sockaddr_in* addr, int port){

    int sd;
    int ret;
    
    sd = socket(AF_INET, SOCK_STREAM, 0);   // Creazione socket
    memset(addr, 0, sizeof(*addr));         // Pulizia

    // Configurazione dell'indirizzo
    (*addr).sin_family = AF_INET;
    (*addr).sin_port = htons(port);
    (*addr).sin_addr.s_addr = INADDR_ANY;

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

    inet_pton(AF_INET, "127.0.0.1", &(*srv_addr).sin_addr);

    return sd;

}

int dev_connect(struct sockaddr_in* dev_addr, int port){

    int sd;

    sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(dev_addr, 0, sizeof(*dev_addr));     // Pulizia 
    (*dev_addr).sin_family = AF_INET;           // Address family
    (*dev_addr).sin_port = htons(port);

    inet_pton(AF_INET, "127.0.0.1", &(*dev_addr).sin_addr);

    return sd;

}

void send_srv(int sd, char* cmd){

    // In questo momento sono connesso al server 
    int ret;
    printf("Invio %s\n", cmd);
    ret = send(sd, cmd, strlen(cmd)+1, 0);

    if(ret < 0){
        perror("Errore in fase di invio comando: ");
        exit(1);
    }
    
    printf("Messaggio %s inviato\n", cmd);
    
}

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
            printf("- Per creare un account digiti signup.\n\n-> ");
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
            printf("- Altrimenti digiti signup per registrare un account.\n\n-> ");
            continue;
        } else{
            // Altrimenti registra l'username e il device lo comunica
            printf("\nUsername online\n\n");
            break;
        }
    }
    return false;

}

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

void second_print(){
    printf("\nSi prega di inserire un comando:\n\n");
    printf("- hanging: per vedere gli utenti che hanno inviato messaggi mentre era offline.\n");
    printf("- show 'username': per ricevere i messaggi pendenti inviati da 'username'.\n");
    printf("- chat 'username': per iniziare una chat con 'username'.\n");
    printf("- out: per disconnettersi dal Server.\n\n");
    printf("(Digitare help per i dettagli dei comandi)\n\n");
}

void chat_config(int sd){

    //FILE* fptr;
    char buffer[1024];
    char dev_usr[1024];
    struct sockaddr_in dev_addr;
    int dev_port;
    int dev_sd;
    int ret;

    // Mandiamo il comando
    send_srv(sd, CMD_CHAT);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, RFD)){
            continue;
        }
        send_srv(sd, username);
        break;
    }

    // Ricevo la rubrica
    recv(sd, rubrica, sizeof(rubrica), 0);

    printf("Rubrica degli utenti registrati nel sistema:\n\n");
    printf("%s\n", rubrica);

    /*
    strcat(percorso, "/");  
    strcat(percorso, "rubrica.txt");
    fptr = fopen(percorso, "w+");
    fflush(fptr);
    fprintf(fptr, "%s", rubrica);
    fclose(fptr);
    */

    while(1){
        printf("Si prega di inserire l'username con cui si vuole aprire una chat\n");
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
        dev_port = atoi(buffer);
        printf("La porta di %s e' %d\n", dev_usr, dev_port);
        break;
    }

    dev_sd = dev_connect(&dev_addr, dev_port);

    // Provo a connettermi al dispositivo
    ret = connect(dev_sd, (struct sockaddr*)&dev_addr, sizeof(dev_addr));

    if(ret < 0){
        printf("Dispositivo offline\n");    
    }
    printf("Online\n");

}

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

    remove(percorso);
    rmdir(username);
}

int main(int argc, char* argv[]){

    int ret, srv_sd, srv_port;
    struct sockaddr_in my_addr, srv_addr, dev_addr;
    char spacenter[1024];
    fd_set master;                                  // Set principale gestito dal programmatore con le macro 
    fd_set read_fds;                                // Set di lettura gestito dalla select 
    int fdmax;                                      // Numero max di descrittori
    int listener;                                   // Socket di ascolto
    int newfd;                                      // Socket di comunicazione
    char buffer[1024];
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
            printf("\n--> Digiti SIGNUP per creare un account.\n\n");
            printf("--> Se ha gia' un account registrato digiti LOGIN.\n\n-> ");
            continue;
        }
    }

    printf("\nDevice ONLINE !\n");

    // Adesso il Device e' online, dobbiamo inviare al Server il timestamp corrente
    online_config(srv_sd, my_port);
    
    printf("Salve %s ! Benvenuto nel sistema di chatting.\n", username);

    // Il Device si e' loggato, bisogna creare il menu' di comparsa
    second_print();

    // Creiamo la cartella dell'username
    mkdir(username, 0700);

    while(1){

        strcpy(percorso, username);
        //strcat(percorso, "/");
        read_fds = master;

        printf("Parte il ciclo e chiamo la select\n");
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        for(i=0; i<=fdmax; i++) {
            // Cerco quelli pronti
            if(FD_ISSET(i, &read_fds)) {
                if(!i){
                    scanf("%s", buffer);

                    if(!strcmp(buffer, "hanging")){
                        // Gestione comando hanging
                        printf("Gestione comando %s\n", buffer);
                    }

                    else if(!strcmp(buffer, "show")){
                        // Gestione comando show
                        printf("Gestione comando %s\n", buffer);
                        scanf("%s", buffer);
                        printf("%s\n", buffer);
                    }

                    else if(!strcmp(buffer, "chat")){
                        // Gestione comando chat
                        printf("Gestione comando %s\n", buffer);
                        chat_config(srv_sd);
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
                    
                    printf("Sono qui !\n");
                    addrlen = sizeof(dev_addr);

                    newfd = accept(listener, (struct sockaddr *)&dev_addr, (socklen_t*)&addrlen);

                    printf("Ho accettato la connessione sul listener, aggiungo nuovo socket al SET\n");

                    FD_SET(newfd, &master);                     // Aggiungo il nuovo socket al master
                    fdmax = (newfd > fdmax) ? newfd : fdmax;    // Aggiorno fdmax

                   
                }
            } 
        }
     }

    sleep(60);

    return 0;

}
