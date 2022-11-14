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

#define STDIN 0
#define RFD "RFD\0"
#define CMD_CHAT "/CHAT\0"
#define CMD_CHATOFF "/CHATOFF\0"
#define CMD_HANGING "/HANGING\0"
#define CMD_LOG "/LOGIN\0"
#define CMD_OFF "/OFF\0"
#define CMD_TMS "/TIMESTAMP\0"
#define CMD_PORT "/PORT\0"
#define CMD_REG "/REG\0"
#define CMD_RUBRIC "/RUBRIC\0"
#define CMD_SHOW "/SHOW\0"
#define YES "/YES\0"
#define NO "/NO\0"
#define EXIT "\\q\0"
#define OFFLINE "/OFFLINE\0"
#define SRV_OUT "/SRV_OUT\0"

char buffer[1024];
char messaggio[1024];
int users = 0;
int sockets[20];
int fdmax;                                      // Numero max di descrittori


void first_print(){
    //int i;
    //printf("\n");
    /*for(i=0; i < 190; i++){
        printf("*");
    }
    printf("\n");
    for (i=0; i < 11; i++){
        printf("\t");
    }*/
    /*for(i=0; i < 190; i++){
        printf("*");
    }*/
    printf("\n\n-> Si prega di inserire un comando:\n");
    printf("\n- list: Mostra gli utenti online.\n");
    printf("- esc:  Chiude il server.\n");
    printf("\n(Digitare 'help' per i dettagli dei comandi)\n\n-> ");
    fflush(stdout);
    

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
    
    sd = socket(AF_INET, SOCK_STREAM, 0);           // Creazione socket
    memset(dev_addr, 0, sizeof(*dev_addr));         // Pulizia

    (*dev_addr).sin_family = AF_INET;               // Address family
    (*dev_addr).sin_port = htons(dev_port);

    return sd;
}

bool check_word(FILE* ptr, char stringa[1024]){

    //printf("Checkiamo la parola %s\n", stringa);

    char buffer[1024];

    while(fscanf(ptr, "%s", buffer)==1){
        //printf("Trovo %s\n", buffer);
        if(!strcmp(buffer, stringa)){
            return true;
        }
    }

    return false;
}

char* filetobuffer(FILE* fptr){

    char scorre[1024];
    //bool timestamp = true;
    memset(buffer, 0, sizeof(buffer));         // Pulizia della variabile globale

    while (fgets(scorre, 1024, fptr) != NULL) {
        strcat(buffer, scorre);
        fflush(fptr);
    }
    /*
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
    */
    return buffer;

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

int count_lines(FILE* fptr){

    int ch = 0;
    int lines = 0;

    fflush(fptr);
    while(!feof(fptr)){
        ch = fgetc(fptr);
        if(ch == '\n'){
            lines++;
        }
    }
    return lines;

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
        //perror("Errore in fase di invio segnale: \n");
        //exit(1);
    }
    
    //printf("Segnale %s inviato\n", cmd);
    
}

/*
void dev_port(int sd){

    char username[1024];
    char port[1024];
    char scorre[1024];

    FILE* fptr = fopen("srv/usr_log.txt", "r");

    send_dv(sd, RFD);
    recv(sd, username, sizeof(username), 0);

    send_dv(sd, RFD);
    recv(sd, port, sizeof(port), 0);

    while(fscanf(fptr, "%s", scorre)==1){
        if(!strcmp(scorre, username)){
            fscanf(fptr, "%s", scorre);
            if(!strcmp(scorre, port)){
                break;
            }
            else{
                send_dv(sd, scorre);
            }
        }
        if(!strcmp(scorre, port)){
            send_dv(sd, NO);
        }
    }

    send_dv(sd, YES);

}
*/

void dev_reg(int sd){

    char username[1024];
    char password[1024];

    while(1){

        FILE* fptr;
        // Ricevo username
        recv(sd, username, sizeof(username), 0);

        //printf("Ricevuto %s\n",username);
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
        //printf("Ricevuto %s\n",password);

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
        //printf("Gestione username\n");
        // Ricevo username
        recv(sd, username, sizeof(username), 0);

        //printf("Ricevuto %s\n",username);

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

            //printf("Gestione password\n");
            recv(sd, password, sizeof(password), 0);
            //printf("Ricevuto %s\n", password);

            if(!strcmp(password, "signup")){
                //printf("Si vuole registrare\n");    
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

    //printf("%s loggato nel sistema\n", username);

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
    //printf("Ricevuto %s\n", username);

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
            //printf("%s\n", buffer);
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

    //printf("Ricevuto %s\n", timestamp);


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
    fflush(fptr);
    
    // Ready for data
    send_dv(sd, RFD);

    // Si riceve l'username che vuole iniziare la chat
    recv(sd, username, sizeof(username), 0);

    memset(lista, 0, sizeof(lista));        // Pulizia

    while(fscanf(fptr, "%s", scorre)==1){

        fpptr = fopen("srv/usr_online.txt", "r");
        fflush(fpptr);
        
        // Se si trova lo stesso username si ignora
        if(!strcmp(scorre, username)){
            continue;
        }

        //printf("Vedo %s\n", scorre);
        fflush(fptr);

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
        fclose(fpptr); 
    }

    fclose(fptr);

    // Inviamo al device la lista
    send_dv(sd, lista);

}

bool unreachable(int sd, char username[1024]){

    FILE* fptr;
    //struct sockaddr_in dev_addr;
    //int dev_sd, ret;
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

    fclose(fptr);

    // Se e' offline, dobbiamo dirlo al device che ha richiesto la prima chat

    fptr = fopen("srv/usr_online.txt", "r");
    fflush(fptr);
    if(!check_word(fptr, username)){
        change_log(username);
        send_dv(sd, OFFLINE);
        fclose(fptr);
        return false;
    }

    fclose(fptr);
    // Altrimenti inviamo la porta del device con cui vuole iniziare la chat
    send_dv(sd, dev_port);
    
    // Il return true sta per -> il device con cui vuoi iniziare la chat e' online
    return true;

}

void dev_rubric(int sd){

    char lista[1024];
    char buffer[1024];
    char scorre[1024];
    char stringa[1024];
    char* users;
    FILE* fptr;
    bool find = false;
    int i = 0;
    int ch = 0;

    // Ready for data
    send_dv(sd, RFD);


    // Si riceve l'username che vuole aggiungere un utente alla chat
    recv(sd, buffer, sizeof(buffer), 0);
    //printf("\nRicevo %s", buffer);
    strcpy(lista, buffer);
    users = lista;

    memset(buffer, 0, sizeof(buffer));

    //strcpy(lista, lista);

    fptr = fopen("srv/usr_online.txt", "r");

    while(fgets(scorre, 1024, fptr) != NULL){
        if(!(i%3)){     // Stiamo visionando un username
            //printf("online_user: %s", scorre);
            ch = 0;
            users = lista;
            //printf("Users: %s", users);
            while(sscanf(users, "%s", stringa)==1){
                //printf("Entro\n");
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

    fclose(fptr);

    //printf("Lista degli utenti online:\n%s", buffer);
    if(buffer[0] == '-'){
        send_dv(sd, buffer);
    } else{
        send_dv(sd, NO);
        return;
    }

    
    recv(sd, buffer, sizeof(buffer), 0);
    //printf("Ricevo %s\n", buffer);

    fptr = fopen("srv/usr_log.txt", "r");
    fflush(fptr);

    while(fscanf(fptr, "%s", stringa)==1){
        if(!strcmp(buffer, stringa)){
            fscanf(fptr, "%s", stringa);
            break;
        }
    }

    send_dv(sd, stringa);

    //printf("Invio %s\n", stringa);


}

void dev_chat_offline(int sd){

    char percorso[1024];
    char dev_usr[1024];
    char buffer[1024];
    char username[1024];
    FILE *fpptr;


    send_dv(sd, RFD);
    recv(sd, dev_usr, sizeof(dev_usr), 0);

    send_dv(sd, RFD);
    recv(sd, username, sizeof(username), 0);

    // Adesso il server deve inoltrare il messaggio al mittente
    send_dv(sd, RFD);
    
    strcpy(percorso, "srv/");
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

    while(1){
        recv(sd, buffer, sizeof(buffer), 0);
        if(!strcmp(buffer, EXIT)){
            return;
        }
        //printf("%s\n", buffer);
        // Si invia il messaggio in una directory contenente i messaggi pendenti
        // percorso -> srv/dev_usr
        fpptr = fopen(percorso, "a");
        fflush(fpptr);
        fprintf(fpptr, "%s\n", buffer);
        fflush(fpptr);
    }

}

void dev_chat(int sd){

    // Preparo la lista degli utenti online e offline
    char username[1024];
    crea_lista(sd, username);
    
    char buffer[1024];
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

        // Prima di tutto controlliamo se il device e' registrato nel sistema
        fptr = fopen("srv/usr_log.txt", "r");
        fflush(fptr);

        // Se non esiste si invia al device NO
        if(!check_word(fptr, dev_usr)){
            send_dv(sd, NO);
            // E si ricevera' un ulteriore username da controllare
            continue;
        }

        fclose(fptr);

        if(unreachable(sd, dev_usr)){
            //printf("ESCO\n");
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

}

char* hang_msg(char percorso[1024]){

    char buffer[1024];
    char dev_usr[1024];
    char timestamp[1024];
    int lines;
    char ch;
    FILE* fptr;
    int i = 1;  // current line
    int j = 0;  // current word
    bool timestamped = false;
    bool dev_usred;

    //printf("%s\n", percorso);

    fptr = fopen(percorso, "r");
    fflush(fptr);
    lines = count_lines(fptr);
    fclose(fptr);
    fptr = fopen(percorso, "r");
    fflush(fptr);
    while(fscanf(fptr, "%s", buffer)==1){
        //printf("buffer = %s, j = %d, i = %d, lines = %d.\n", buffer, j, i, lines);
        if(i==lines && !timestamped){   // Se siamo nell'ultima linea
            // Estraiamo il timestamp, che è la prima stringa 
            strcpy(timestamp, buffer);
            timestamped = true;
        }

        if(++j == 2 && !dev_usred){
            strcpy(dev_usr, buffer);
            dev_usred = true;
        }

        ch = fgetc(fptr);
        if(ch == '\n'){         // Se si va a capo si incrementa la linea
            i++;
        }
    }

    memset(messaggio, 0, sizeof(messaggio));

    sprintf(messaggio, "\nMessaggi ricevuti da %s %d.\n", dev_usr, lines);
    sprintf(buffer, "Timestamp del piu' recente: %s.\n", timestamp);
    strcat(messaggio, buffer);

    //printf("%s\n", messaggio);

    return messaggio;

}

void dev_hanging(int sd){

    char dev_usr[1024];
    char username[1024];
    char percorso[1024];
    char filename[1024];
    char buffer[1024];
    char msg[1024];
    int i = 0;
    bool empty = false;
    FILE* fptr, *fpptr;
    memset(msg, 0, sizeof(msg));
    memset(buffer, 0, sizeof(buffer));
    strcpy(percorso, "srv/");

    send_dv(sd, RFD);

    recv(sd, username, sizeof(username), 0);

    strcat(percorso, username);
    strcat(percorso, "/pendent/");

    
    fptr = fopen("srv/usr_all.txt", "r");
    fflush(fptr);

    while(fscanf(fptr, "%s", dev_usr)==1){
        
        strcpy(buffer, percorso);

        strcpy(filename, dev_usr);
        strcat(filename, ".txt");
        
        strcat(buffer, filename);

        //printf("%s\n", buffer);

        fpptr = fopen(buffer, "r"); 
        
        if(!fpptr){
            if(!i){
                //printf("true\n");
                empty = true;
            }
            continue;
        }

        fclose(fpptr);

        // Questa funzione prepara il messaggio da inviare al Device
        strcat(msg, hang_msg(buffer));
        empty = false;     
        i++;   
    }
    if(empty){
        strcpy(msg, "\nNessun messaggio pendente !\n");
    }
    send_dv(sd, msg);

}

void dev_show(int sd){

    char dev_usr[1024];
    char username[1024];
    char percorso[1024];
    //char filename[1024];
    //char buffer[1024];
    char msg[1024];
    FILE* fptr;

    send_dv(sd, RFD);

    recv(sd, username, sizeof(username), 0);

    send_dv(sd, RFD);

    recv(sd, dev_usr, sizeof(dev_usr), 0);

    fptr = fopen("srv/usr_all.txt", "r");
    fflush(fptr);

    if(!check_word(fptr, dev_usr)){
        send_dv(sd, NO);
        return;
    }

    fclose(fptr);

    strcpy(percorso, "srv/");
    strcat(percorso, username);
    strcat(percorso, "/pendent/");
    strcat(percorso, dev_usr);
    strcat(percorso, ".txt");

    //printf("Il percorso e' %s\n", percorso);

    fptr = fopen(percorso, "r");
    if(!fptr){
        send_dv(sd, NO);
        return;
    }
    
    fflush(fptr);
    strcpy(msg, filetobuffer(fptr));
    send_dv(sd, msg);
    remove(percorso);

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

    //printf("%s sta andando OFFLINE !\n", username);

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
        //printf("%s\n", buffer);
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
    char ch;

    FILE* fptr = fopen("srv/usr_online.txt", "r");

    ch = fgetc(fptr);
    if(ch == -1){
        printf("\n---> ATTENZIONE ! Nessun utente e' attualmente online.");
        fflush(stdout);
        return;
    }

    printf("\nLista degli utenti online:\n\n");

    printf("%c", ch);
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
    printf("\t- Un Device non puo' loggarsi con il Server spento.\n");
    
}

void srv_out(){

    int i;
    FILE* fptr = fopen("srv/usr_online.txt", "w+");
    fclose(fptr);
    for(i=0; i <=users; i++){
        send_dv(sockets[i], SRV_OUT);
    }
    exit(0);

}

void handler(int sig){

    srv_out();

}

int main(int argc, char *argv[]) {
    
    // Dichiarazioni Variabili
    int port;                                       // Porta a cui e' associato il Server
    int ret;                                        // Variabile di controllo
    fd_set master;                                  // Set principale gestito dal programmatore con le macro 
    fd_set read_fds;                                // Set di lettura gestito dalla select 
    struct sockaddr_in my_addr;                     // Indirizzo Server
    struct sockaddr_in cl_addr;                     // Indirizzo Device 
    int listener;                                   // Socket di ascolto
    int newfd;                                      // Socket di comunicazione
    char command[1024];
    char buffer[1024];
    int i;
    int addrlen;

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

    // Handler per la gestione della disconnessione del device improvvisa
    signal(SIGINT, handler);    // CTRL+C

    // Ciclo principale
    while(1){

        read_fds = master;
        //printf("Parte il ciclo e chiamo la select\n\n");
        
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        for(i=0; i<=fdmax; i++) {
            // Cerco quelli pronti
            if(FD_ISSET(i, &read_fds)) {                    // Trovato socket pronto

                if(!i){                                     // Quello pronto riguarda la stdin
                    scanf("%s", buffer);
                    if(!strcmp(buffer, "list")){
                        // Gestione comando list
                        srv_list();
                        first_print();
                    }

                    else if(!strcmp(buffer, "esc")){
                        // Gestione comando esc
                        srv_out();
                    }

                    else if(!strcmp(buffer, "help")){
                        // Gestione comando help
                        srv_help();
                        first_print();
                    }

                    else{
                        printf("\nATTENZIONE ! Comando -%s- inesistente.", buffer);
                        first_print();
                    }
                }

                else if(i == listener) {                    // Se quello pronto e' il listener
                    
                    addrlen = sizeof(cl_addr);

                    newfd = accept(listener, (struct sockaddr *)&cl_addr, (socklen_t*)&addrlen);

                    //printf("Ho accettato la connessione sul listener, aggiungo nuovo socket al SET\n");

                    FD_SET(newfd, &master);                     // Aggiungo il nuovo socket al master
                    fdmax = (newfd > fdmax) ? newfd : fdmax;    // Aggiorno fdmax

                   
                } else{                                     // Se il socket pronto e' il comunicatore

                while(1){
                    ret = recv(i, command, sizeof(command), 0);   

                    if(!ret){                               // Socket i e' stato chiuso, Device offline
                        //printf("Socket chiuso\n");
                        FD_CLR(i, &master);                 // Lo tolgo dal master 
                        close(i);                           // Lo chiudo
                    } 
                    else if(ret > 0){                     // Qui arriva il SEGNALE /XXX

                        //printf("Il comunicatore (socket %d) e' pronto\n", i);
                        //printf("E' arrivato il comando %s\n", command);

                        // Gestione registrazione
                        if(!strcmp(command,CMD_REG)){
                            // La funzione si occupa della corretta registrazione del device
                            // Prende in ingresso il socket descriptor
                            //printf("Gestione registrazione\n");
                            dev_reg(i);
                        }

                        // Gestione login
                        else if(!strcmp(command, CMD_PORT)){
                            //printf("Gestione port\n");
                            //dev_port(i);
                        }

                        // Gestione login
                        else if(!strcmp(command, CMD_LOG)){
                            //printf("Gestione login \n");
                            // La funzione si occupa del login del device
                            // Prende in ingresso il socket descriptor
                            if(dev_log(i)){
                                continue;
                            }
                        }

                        // Gestione timestamp
                        else if(!strcmp(command, CMD_TMS)){
                            //printf("Gestione timestamp\n");
                            dev_online(i);
                            sockets[users++] = i;
                        }

                        // Gestione chat
                        else if(!strcmp(command, CMD_CHAT)){
                            //printf("Gestione chat\n");
                            dev_chat(i);
                        }
                        
                        else if(!strcmp(command, CMD_RUBRIC)){
                            //printf("Gestione rubric\n");
                            dev_rubric(i);
                        }

                        else if(!strcmp(command, CMD_HANGING)){
                            //printf("Gestione hanging\n");
                            dev_hanging(i);
                        }

                        else if(!strcmp(command, CMD_SHOW)){
                            //printf("Gestione show\n");
                            dev_show(i);
                        }

                        else if(!strcmp(command, CMD_OFF)){
                            //printf("Gestione out\n");
                            dev_out(i);
                            //printf("Socket chiuso\n");
                            FD_CLR(i, &master);                 // Lo tolgo dal master 
                            close(i);                           // Lo chiudo
                        }

                        else if(!strcmp(command, CMD_CHATOFF)){
                            //printf("Gestione chat offline\n");
                            dev_chat_offline(i);
                        }

                    } 
                    else{
                        //perror("Errore nella reiceve: ");
                    }
                    break;
                } 
                }     
            }
        }
    }
    return 0;
}
