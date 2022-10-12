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

int main(int argc, char* argv[]){

    int ret, sd, len, port;
    struct sockaddr_in my_addr;
    char buffer[1024];
    int i = 0;

    if(argc < 2){
        printf("Inserisci una porta\n");
        exit(-1);
    }

    port = atoi(argv[1]);

    // Prima stampa
    first_print();

    // Si connette il device alla porta desiderata 
    sd = ip_config(&my_addr, port);

    scanf("%s", buffer);




}