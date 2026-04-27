#include "../include/protocol.h"
#include <string.h>

//funzione generica condivisa tra server e client per inviare un pacchetto
void send_packet(int socket,Packet *p,struct sockaddr_in *dest){
    sendto(socket,p,sizeof(Packet),0,(struct sockaddr*) dest,sizeof(dest));
}

//copia la carta src in dst
void copy_card(Card src,Card dst){
    dst.col=src.col;
    dst.id=src.id;
    dst.next=src.next;
    strcpy(dst.text,src.text);
    dst.timestamp=src.timestamp;
    dst.user_port=src.user_port;
}
//fornisce una rappresentazione grafica della carta
void print_card(Card c){ 
    int len=strlen(c.text);
    for(int i=0;i<24;i++){
        printf("-");
    }
    printf("\n");
    printf("ID: %i\n\n",c.id);
    printf('"');
    for(int i=0;i<len;i++){
        if(i%20==0){
            printf("-  ");
            printf("\n");
            printf(". -");

        }
        printf("%c",c.text[i]);
    }
    printf('"\n');
    for(int i=0;i<24;i++){
        printf("-");
    }
    printf("\n\n");
}