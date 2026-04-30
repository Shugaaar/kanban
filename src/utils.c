#include "../include/protocol.h"
#include <string.h>

/*
Questo modulo contiene funzioni utili per entrami i moduli utente e lavagna e quindi è giusto che siano messe a comune 
evitando ridondanze di codice
*/

/**
 * @brief invia un pacchetto generico supportato dal programma
 * @param socket di invio
 * @param puntatore al pacchetto da inviare
 * @param indirizzo di destinazione
 */
void send_packet(int socket,Packet *p,struct sockaddr_in *dest){
    printf("Invio %s a %s:%i\n",command_names[p->cmd],inet_ntoa(dest->sin_addr), ntohs(dest->sin_port));
    int sent= sendto(socket,p,sizeof(Packet),0,(struct sockaddr*) dest,sizeof(struct sockaddr_in));
    if(sent<0){
        perror("Errore send_packet");
    }else{
        //printf("Inviati %i byte\n",sent);
    }
}

/**
 * @brief assegna i parametri passati alla carta
 * @param puntatore alla carta
 */
void create_card(struct Card* c,int id,ColumnType col,const char text[256]){
    c->id=id;
    c->col=col;
    c->user_port=0;
    strcpy(c->text,text);
    c->next=NULL;
    c->timestamp=time(NULL);
}

/**
 * @brief copia la carta src in dst
 */
void copy_card(struct Card src,struct Card* dst){
    dst->col=src.col;
    dst->id=src.id;
    dst->next=src.next;
    strcpy(dst->text,src.text);
    dst->timestamp=src.timestamp;
    dst->user_port=src.user_port;
}

/**
 * @brief fornisce una rappresentazione grafica su terminale della carta
 */
void print_card(struct Card c){ 
    int len=strlen(c.text);
    for(int i=0;i<(WIDTH_LAVAGNA/3);i++){
        printf("-");
    }
    printf("\n");
    printf("ID: %i\n\n",c.id);
    for(int i=0;i<len;i++){
        if(i%WIDTH_LAVAGNA==0){
            
            printf("\n");
            printf("-  ");

        }
        printf("%c",c.text[i]);
    }
    printf("\n");
    for(int i=0;i<(WIDTH_LAVAGNA/3);i++){
        printf("-");
    }
    printf("\n\n");
}

