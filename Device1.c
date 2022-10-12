#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define BUFFER_SIZE 1024
#define SIG_UP "LOGIN\0"
#define SIG_RFD "RFD\0"

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

    printf("\nSi prema spazio per fare login o enter per creare un account\n");
       
    
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

    int sd, ret;

    sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(srv_addr, 0, sizeof(*srv_addr)); // Pulizia 
    (*srv_addr).sin_family = AF_INET;          // Address family
    (*srv_addr).sin_port = htons(4242);

    inet_pton(AF_INET, "127.0.0.1", &(*srv_addr).sin_addr);

    return sd;

}

int main(int argc, char* argv[]){

    int ret, my_sd, len, port, srv_sd;
    struct sockaddr_in my_addr, srv_addr;
    char spacenter[1024];
    char buffer[1024];
    char username[1024];
    int i = 0;

    if(argc < 2){
        printf("Inserisci una porta\n");
        exit(-1);
    }

    port = atoi(argv[1]);


    // Si connette il device alla porta desiderata 
    my_sd = ip_config(&my_addr, port);

    // Prima stampa
    first_print();
    scanf("%s", spacenter);

    // Ci si connette al server
    srv_sd = srv_config(&srv_addr);

    while(1){
        if(spacenter[0] == '\n'){
            printf("Gestione creazione account\n");
            break;
        } else if(spacenter[0] == ' '){
            printf("Gestione login");
        }
        else{
            continue;
        }
    }

    // Si manda un segnale al Server per notificare che l'utente e' online
    // La funzione hey_server() e' una booleana che restituisce true nel caso di
    // primo accesso in assoluto, altrimenti false
    /*

            printf("Inserisci un username\n");
            scanf("%s", username);
            if(hey_server(other signal)){
                printf("Username gia' utilizzato\n");
            }

        }
    }
    */


    return 0;

}