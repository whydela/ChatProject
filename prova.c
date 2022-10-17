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

int main(){
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    // Converto l'ora
    timeinfo = localtime(&rawtime);

    printf("%d\n", timeinfo->tm_year+1900);
}