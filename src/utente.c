#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../include/types.h"
#include "../include/config.h"
#include "../include/protocol.h"
#include "utils.c"

int main(int argc,char* argv[]){
    if(argc !=2){
        fprintf(stderr,"Uso: %s [porta utente]\n",argv[0]);
        return 1;
    }
    int my_port=atoi(argv[1]);
    if(my_port<UTENTE_PORT_START||my_port>UTENTE_PORT_START+MAX_UTENTI){
        fprintf(stderr,"La porta utente dev'essere compresa tra %i e %i\n",UTENTE_PORT_START,UTENTE_PORT_START+MAX_UTENTI);
        return 1;
    }

    int sockfd;

    //creo il socket di tipo UDP
    if(sockfd=socket(AF_INET,SOCK_DGRAM,0)<0){
        perror("Errore durante la creazione del socket\n");
        return 1;
    }

    //assegno un indirizzo al socket
    struct sockaddr_in my_addr;
    my_addr.sin_family=AF_INET;
    my_addr.sin_addr.s_addr=INADDR_ANY;
    my_addr.sin_port=htons(my_port);

    if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(my_addr))<0){
        perror("Errore nella bind");
        return 1;
    }
    
    //creo il socket della lavagna
    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(LAVAGNA_PORT);
    inet_pton(AF_INET,SERVER_IP,&server_addr.sin_addr); //converto e assegno l'ip al socket della lavagna

    //creo il pacchetto per mandare l'HELLO
    Packet pkt;
    pkt.cmd=HELLO;
    pkt.sender_port=my_port;
    send_packet(sockfd,&pkt,&server_addr);
    printf("Utente porta %d: Registrato alla lavagna.\n", my_port);

    //DEVO CREARE UNA VERIFICA CHE L'HELLO SIA ARRIVATO

    printf("Utente porta %d: Registrato alla lavagna.\n", my_port);

    while(1){ //loop principale, attendo che arrivi un pacchetto e agisco di conseguenza

    }


}