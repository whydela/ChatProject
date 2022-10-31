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

#define RFD "RFD\0"
#define CMD_REG "/REG\0"
#define CMD_LOG "/LOGIN\0"
#define CMD_TMS "/TIMESTAMP\0"
#define CMD_CHAT "/CHAT\0"
#define CMD_OFF "/OFF\0"
#define CMD_CHATOFF "/CHATOFF\0"
#define CMD_HANGING "/HANGING\0"
#define CMD_SHOW "/SHOW\0"
#define YES "/YES\0"
#define NO "/NO\0"
#define OFFLINE "/OFFLINE\0"
#define STDIN 0
#define EXIT "\\q\0"

char username[1024];                    // username del device
char password[1024];                    // password del device
char timestamp[1024];                   // username*timestamp*porta del device
char percorso[1024];                    // percorso: username/file.txt
char rubrica[1024];                     // rubrica
char pendent[1024];
char messaggio[1024];                   // messaggio della chat

int my_port;                            // porta del device
int srv_sd;
int dev_sd;
bool online = false;
bool chatting = false;


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

// Cleaner che cambia il singolo asterisco con quello doppio nelle chat
void cleaner(char filename[1024]){
    
    FILE* fptr, *fpptr;
    char buffer[1024];
    char file[1024];
    char file1[1024];

    strcpy(file, filename);
    strcpy(file1, filename);

    strcat(file, ".txt");
    strcat(file1, "1.txt");
    printf("%s\n", file);
    printf("%s\n", file1);

    fpptr = fopen(file, "a+");
    fptr = fopen(file1, "a+");

    fflush(fpptr);
    fflush(fptr);
    while(fscanf(fpptr, "%s", buffer)==1){
        //printf("Trovo %s\n", buffer);
        fflush(fptr);
        if(!strcmp(buffer, "*")){
            fprintf(fptr, "**\n");
        } else if(!strcmp(buffer, "**")){
            fprintf(fptr, "**\n");
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
    printf("- chat: per iniziare una chat.\n");
    printf("- out: per disconnettersi dal Server.\n\n-> ");
}

// La funzione sottostante prepara il messaggio da inviare nella chat
char* msg(bool online){

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
    strcat(messaggio, "\t");
    strcat(messaggio, username);
    strcat(messaggio, ": ");

    scanf("%s", stringa);

    if(!strcmp(stringa, EXIT)){
        return EXIT;
    }

    fgets(buffer, sizeof(buffer), stdin);
    buffer[strlen(buffer)-1]= '\0';
    strcat(stringa, buffer);
    strcat(messaggio, stringa);
    
    if(online){
        strcat(messaggio, " **");
    } else{
        strcat(messaggio, " *");
    }

    return messaggio;

}

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
            strcpy(buffer, msg(false));
            send_srv(sd, buffer);
            if(!strcmp(buffer, EXIT)){
                break;
            }
            else{
                printf("%s\n", buffer);
                fflush(fptr);
                fprintf(fptr, "%s\n", buffer);
                fflush(fptr);
            }
        }
        break;
    }    

}  

// La funzione sottostante si occupa della chat di entrambi i Device
void chat(int sd, char dev_usr[1024]){

    fd_set master_chat;
    fd_set read_chat;
    int fdmax;
    int i;
    bool online = true;
    chatting = true;
    dev_sd = sd; 

    FD_ZERO(&read_chat);
    FD_ZERO(&master_chat);
    FD_SET(STDIN, &master_chat);
    FD_SET(sd, &master_chat);

    fdmax = sd;
    //FILE* fptr;
    char sent[1024];
    char coming[1024];
    char buffer[1024];
    int ret;
    FILE* fptr;

    strcpy(buffer, username);
    strcat(buffer, "/chat");
    mkdir(buffer, 0700);
    strcat(buffer, "/");
    strcat(buffer, dev_usr);
    cleaner(buffer);
    strcat(buffer, ".txt");
    printf("Il percorso e' %s\n", buffer);

    printf("Chat con %s iniziata !\n\n", dev_usr);
    printf("--->");
    printf(" Per uscire dalla chat digitare '\\q'\n");
    printf("--->");
    printf(" Per aggiungere un partecipante digitare '\\u'\n");
    printf("--->");
    printf(" Per condividere un file digitare 'share nomefile'\n");
    printf("--->");
    printf(" Per uscire dalla chat digitare '\\q'\n\n");

    fptr = fopen(buffer, "a");
    fflush(fptr);

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
                    strcpy(sent, msg(online));

                    ret = send_dev(sd, sent);

                    if(ret > 0){
                        printf("%s\n", sent);
                        if(strcmp(sent, EXIT)){
                            fflush(fptr);   
                            fprintf(fptr, "%s\n", sent);
                            fflush(fptr);
                        }
                    }

                    if(!strcmp(sent, EXIT)){
                        printf("VOGLIO USCIRE\n\n");
                        chatting = false;
                        fclose(fptr);
                        return;
                    }
                }

                else if(i == sd){

                    ret = recv(sd, coming, sizeof(coming), 0);

                    if (ret > 0){
                        fflush(stdin);
                        fflush(stdout);
                        if(!strcmp(coming, EXIT)){
                            printf("ATTENZIONE ! %s e' uscito dalla chat.\n\n", dev_usr);
                            online = false;
                            close(i);
                            offline_chat(srv_sd, dev_usr, fptr);
                            return;
                        } else{
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

            send_srv(sd, msg(false));
            // Il NO indica che e' la prima volta che questo utente vuole parlare con lui
            // send_srv(sd, NO);
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

    dev_sd = dev_connect(&dev_addr, dev_port);

    // Provo a connettermi al dispositivo
    ret = connect(dev_sd, (struct sockaddr*)&dev_addr, sizeof(dev_addr));

    if(ret < 0){
        // Dispositivo offline
        printf("Dispositivo offline\n");
        return;
        // dev_online = false;
    }

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

void show_config(int sd){

    char buffer[1024];

    send_srv(sd, CMD_SHOW);

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, RFD)){
            send_srv(sd, username);
            break;
        }
    }

    recv(sd, buffer, sizeof(buffer), 0);

    printf("%s", buffer);

}

void handler(int sig){
    
    //printf("Entro nell'handler\n");

    if(chatting){
        send_dev(dev_sd, EXIT);
    }

    if(online){
        out_config(srv_sd);
        printf("\n");
        exit(0);
    }
}


int main(int argc, char* argv[]){

    int ret, srv_port;
    struct sockaddr_in my_addr, srv_addr, dev_addr;
    char spacenter[1024];
    fd_set master;                                  // Set principale gestito dal programmatore con le macro 
    fd_set read_fds;                                // Set di lettura gestito dalla select 
    int fdmax;                                      // Numero max di descrittori
    int listener;                                   // Socket di ascolto
    int newfd;                                      // Socket di comunicazione
    char buffer[1024];
    char command[1024];
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


    // Adesso il Device e' online, dobbiamo inviare al Server il timestamp corrente
    online_config(srv_sd, my_port);

    online = true;

    // Handler per la gestione della disconnessione del device improvvisa
    signal(SIGINT, handler);    // CTRL+C
    signal(SIGSTOP, handler);   // CTRL+Z o chiusura terminale

    printf("Salve %s ! Benvenuto nel sistema di chatting.\n", username);

    // Creiamo la cartella dell'username
    mkdir(username, 0700);
    strcpy(pendent, username);
    strcat(pendent, "/pendent");
    rmdir(pendent);
    //mkdir(pendent, 0700);


    while(1){

        strcpy(percorso, username);
        //strcat(percorso, "/");
        read_fds = master;
        // Il Device si e' loggato, bisogna creare il menu' di comparsa
        second_print();
        fflush(stdout);

        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        for(i=0; i<=fdmax; i++) {
            // Cerco quelli pronti
            if(FD_ISSET(i, &read_fds)) {
                if(!i){
                    scanf("%s", buffer);

                    if(!strcmp(buffer, "hanging")){
                        // Gestione comando hanging
                        printf("Gestione comando %s\n", buffer);
                        hanging_config(srv_sd);
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
  
                    addrlen = sizeof(dev_addr);

                    newfd = accept(listener, (struct sockaddr *)&dev_addr, (socklen_t*)&addrlen);

                    FD_SET(newfd, &master);                     // Aggiungo il nuovo socket al master
                    fdmax = (newfd > fdmax) ? newfd : fdmax;    // Aggiorno fdmax
                   
                }
                else{                                     // Se il socket pronto e' il comunicatore

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
                        //printf("E' arrivato il comando %s\n", command);

                        // Gestione registrazione
                        if(!strcmp(command, CMD_CHAT)){

                            send_dev(i, RFD);

                            recv(i, buffer, sizeof(buffer), 0);

                            printf("%s vuole iniziare una chat con te !\n\n", buffer);

                            printf("---> Digita qualsiasi cosa per entrare nella chat.\n");
                            printf("---> Se si vuole rifutare la chat digitare '\\q'.\n\n");

                            scanf("%s", wait);

                            if(!strcmp(wait, EXIT)){
                                send_dev(i, NO);
                                break;
                            }

                            send_dev(i, RFD);

                            //printf("Gestione CHAT\n");
                            //printf("Hai un nuovo messaggio !\n");
                            chat(i, buffer);

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

     }

    sleep(60);

    return 0;

}
