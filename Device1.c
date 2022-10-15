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

#define BUFFER_SIZE 1024
#define SIG_RFD "RFD\0"
#define CMD_REG "/REG\0"
#define CMD_LOG "/LOGIN\0"
#define YES "/YES\0"
#define NO "/NO\0"

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
        printf("Si prega di inserire una porta corretta\n");
        exit(-1); 
    }

    return sd;

}

int srv_config(struct sockaddr_in* srv_addr){

    int sd;

    sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(srv_addr, 0, sizeof(*srv_addr)); // Pulizia 
    (*srv_addr).sin_family = AF_INET;          // Address family
    (*srv_addr).sin_port = htons(4242);

    inet_pton(AF_INET, "127.0.0.1", &(*srv_addr).sin_addr);

    return sd;

}

void send_srv(int sd, char* cmd){

    // In questo momento sono connesso al server 
    int ret;

    ret = send(sd, cmd, strlen(cmd)+1, 0);

    if(ret < 0){
        perror("Errore in fase di invio comando: ");
        exit(1);
    }
    
    printf("Messaggio %s inviato\n", cmd);
    
}

void reg_config(int sd){

    char username[1024];
    char password[1024];
    char buffer[1024];
    char all[1024];

    send_srv(sd, CMD_REG);

    while(1){
        printf("Inserisca un username:\n-> ");
        scanf("%s", username);
        //Mando l'username al server, lui controllera' se va bene
        send_srv(sd, username);
        // Ricevo la risposta se va bene o no
        recv(sd, buffer, sizeof(buffer), 0);
        // Se il server trova quell'username gia' in uso
        if(!strcmp(buffer, YES)){
            printf("\nATTENZIONE! Username gia' in uso\n\n");
            continue;
        } else{
            break;
        }
    }

    printf("Inserisca una password:\n-> ");
    scanf("%s", password);
    
    // Invio al server l'username e la password insieme
    strcat(username, password);
    printf("%s\n", username);
    send_srv(sd, username);

}

bool log_config(int sd){

    char username[1024];
    char password[1024];
    char buffer[1024];
    //char all[1024];
    int i = 0;

    send_srv(sd, CMD_LOG);

    printf("Inserisca l'username:\n-> ");

    while(1){

        scanf("%s", username);
        i++;
        //printf("%d", i);
        // Mando l'username al server, lui controllera' se va bene
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
            printf("- Per creare un account digiti signup.\n-> ");
            continue;
        } else{
            // Altrimenti registra l'username e il device lo comunica
            break;
        }
    }

    printf("Inserisca la password:\n-> ");

    while(1){

        scanf("%s", password);


        //Mando password ed username al server, lui controllera' se va bene
        //sprintf(all, "%s%s", username, password);
        strcat(password, username);
        printf("%s\n", password);
        send_srv(sd, username);
        // Ricevo la risposta se va bene o no
        recv(sd, buffer, sizeof(buffer), 0);

        // Se il server NON trova quell'username
        if(!strcmp(buffer, NO)){
            printf("\nATTENZIONE! Password non corretta.\n\n");
            printf("- Si prega di inserire una password corretta:\n");
            continue;
        } else{
            // Altrimenti registra l'username e il device lo comunica
            printf("\nUsername online\n\n");
            break;
        }
    }
    
    return false;

}

int main(int argc, char* argv[]){

    int ret, my_sd, port, srv_sd;
    struct sockaddr_in my_addr, srv_addr;
    char spacenter[1024];
    //char buffer[1024];

    if(argc < 2){
        printf("Inserisca una porta\n");
        exit(-1);
    }

    // Porta passata dalla linea di comando
    port = atoi(argv[1]);

    // Si connette il device alla porta desiderata 
    my_sd = ip_config(&my_addr, port);
    if(my_sd > 10){}

    // Prima stampa
    first_print();

    // Configuriamo il socket connesso al server
    srv_sd = srv_config(&srv_addr);

    // Connettiamo il Device al Server
    ret = connect(srv_sd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));

    if(ret < 0){
        printf("\n\n\t\tSERVER OFFLINE !\n");
        exit(-1);
    }

    //printf("\nDevice connesso al server\n");  

    printf("\n--> Digiti SIGNUP per creare un account.\n\n");
    printf("--> Se ha gia' un account registrato digiti LOGIN.\n\n-> ");

    while(1){       
        
        scanf("%s", spacenter);
        if(!strcmp(spacenter, "signup") || !strcmp(spacenter,"SIGNUP")){
            printf("\nREGISTRAZIONE ACCOUNT IN CORSO\n\n");
            // La funzione si occupa di tutta la fase di registrazione comprensiva di richiesta di username e password
            reg_config(srv_sd);
            printf("\nIL SUO ACCOUNT E' STATO REGISTRATO CORRETTAMENTE\n\n");
            break;

        } else if(!strcmp(spacenter, "login") || !strcmp(spacenter, "LOGIN")){
            printf("Gestione login\n");
            // La funzione si occupa di tutta la fase di login
            // E' una booleana: se ritorna true vuol dire che l'utente vuole fare la signup
            if(log_config(srv_sd)){
                    printf("\nREGISTRAZIONE ACCOUNT IN CORSO\n\n");
                    // La funzione si occupa di tutta la fase di registrazione comprensiva di richiesta di username e password
                    reg_config(srv_sd);
                    printf("\nIL SUO ACCOUNT E' STATO REGISTRATO CORRETTAMENTE\n\n");
            }
            break;
        }
        else{
            printf("\nATTENZIONE ! Comando -%s- non riconosciuto.\n", spacenter);
            printf("\n--> Digiti SIGNUP per creare un account.\n\n");
            printf("--> Se ha gia' un account registrato digiti LOGIN.\n\n");
            continue;
        }
    }

    sleep(60);



    return 0;

}
