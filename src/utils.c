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
    for(int i=0;i<(WIDTH_LAVAGNA/3)+4;i++){
        printf("-");
    }
    printf("\n");
    printf("ID: %i\n\n",c.id);
    printf('"');
    for(int i=0;i<len;i++){
        if(i%WIDTH_LAVAGNA==0){
            printf("-  ");
            printf("\n");
            printf(". -");

        }
        printf("%c",c.text[i]);
    }
    printf('"\n');
    for(int i=0;i<(WIDTH_LAVAGNA/3)+4;i++){
        printf("-");
    }
    printf("\n\n");
}

//fornisce una rappresentazione grafica della lavagna
void print_lavagna(Lavagna l){

    for(int i=0;i<WIDTH_LAVAGNA+4;i++){
        printf("-");
    }
    printf("\n   LAVAGNA - ID:%i\n",l.id);

    for(int i=0;i<WIDTH_LAVAGNA+4;i++){
        printf("-");
    }
    char s1[]="    To Do";
    printf("%s",s1);
    for(int i=0;i< (WIDTH_LAVAGNA/3)-strlen(s1);i++){
        printf(" ");
    }
    printf("--");

    char s2[]="    Doing";
    printf("%s",s2);
    for(int i=0;i< (WIDTH_LAVAGNA/3)-strlen(s2);i++){
        printf(" ");
    }
    printf("--");

    char s3[]="    Done";
    printf("%s",s3);
    for(int i=0;i< (WIDTH_LAVAGNA/3)-strlen(s3);i++){
        printf(" ");
    }
    printf("-\n\n");

    for(int i=0;i<(WIDTH_LAVAGNA/3)+4;i++){
        printf("-");
    }

    Card* todo=l.col[0];
    Card* doing=l.col[1];
    Card* done=l.col[2];

    while(todo||doing||done){

        if(todo){
            print_card(*todo);
            todo=todo->next;
        }
        if(doing){
            print_card(*doing);
            doing=doing->next;
        }
        if(done){
            print_card(*done);
            done=done->next;
        }

    }

}