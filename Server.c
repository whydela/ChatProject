#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>




int main(int argc, char *argv[])
{
    // Dichiarazioni Variabili

    int port;                                       // Porta a cui e' associato il Server
    int ret;                                        // Variabile di controllo

    // Si inizializza la porta del Server
    port = (argc > 1) ? atoi(argv[1]) : 4242;



    
    printf("La porta del Server e' %d\n", port);
    return 0;
}
