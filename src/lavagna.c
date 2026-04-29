#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../include/types.h"
#include "../include/config.h"
#include "../include/protocol.h"
#include "utils.c"


/**
 * @brief vettore che contiene le porte degli utenti registrati
 */
int ports[MAX_UTENTI];
/**
 * @brief numero di utenti registrati
 */
int n_users=0;
/**
 * @brief vettore che contiene l'id della carta in doing dall'utente con porta i+USER_START_PORT
 */
int doing[MAX_UTENTI];

/**
 * @brief struttura dati della lavagna
 * 
 */
typedef struct{
    int id;

    /** vettore di 3 puntatori a card, implementa 3 code (TODO,DOING,DONE) di card, utile per evitare di allocare spazio non 
    necessario e gestire comodamente l'ordine di arrivo */
    struct Card* col[3]; 

    /** vettore di Card che contiene tutte le card della lavagna */
    struct Card cards[MAX_CARDS];


} Lavagna;

Lavagna l;

/**
 * @brief agiunge una carta alla lavagna, verifica che la carta non esista già
 * @param carta da aggiungere
 * @return 0 se la carta esiste già, 1 altrimenti
 */
bool add_card(struct Card carta){
    struct Card* c=&l.cards[carta.id];
    if(c->col!=EMPTY)
        return 0;
    
    //aggiorno la carta
    copy_card(carta,c);
    c->timestamp=time(NULL);
    
    //inserisco la carta in cima alla coda 
    c->next=l.col[c->col];
    l.col[c->col]=c;

    return 1;
}

/**
 * @brief estrae la carta da una colonna
 * @param id della carta
 * @param colonna 
 */
void extract_card(int id,ColumnType col){
    //classico codice di rimozione da una coda
    struct Card* head=l.col[col];
    struct Card* c=&l.cards[id];
    if(head->id==id){
        head=NULL;
        return;
    }
    struct Card* prev;
    struct Card* cur;
    for(cur=head;cur->id!=id&&cur!=NULL;cur=cur->next){
        prev=cur;
    }
    if(cur==NULL)
        return;
    prev->next=c->next;  
}

/**
 * @brief sposta la carta in un' altra colonna, verifica che la carta esista
 * @param id della crta
 * @param colonna nella quale si vuole spostare la carta
 * @return 0 se la carta non esisteva, 1 altrimenti
 */
bool move_card(int id,ColumnType col){
    struct Card* c=&l.cards[id];

    if(c->col==EMPTY)   
        return 0;
    //estraggo la carta dalla precedente coda
    extract_card(id,c->col);
    //aggiorno la testa della nuova coda
    c->next=l.col[col]; 
    l.col[col]=c;
    //aggiorno la carta
    c->col=col;
    c->timestamp=time(NULL);

    return 1;
}

/**
 * @brief inizializza la lavagna
 */
void init_lavagna(){
    l.id=1;
    l.col[0]=l.col[1]=l.col[2]=NULL;

    //inizializzo il doing a -1, il che indica che gli uetnti non stanno eseguendo alcuna attività
    for(int i=0;i<MAX_UTENTI;i++){
        doing[i]=-1;
    }

    //creo alcune carte di esempio
    for(int i=0;i<5;i++){
        //char text[256]="attività #"+i;

        struct Card c;
        create_card(&c,i,TODO,"attività #"+i);
        add_card(c);
    }

}

/**
 * @brief fornisce una rappresentazione grafica da terminale della lavagna
 */
void print_lavagna(){

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

    struct Card* todo=l.col[0];
    struct Card* doing=l.col[1];
    struct Card* done=l.col[2];

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

/**
 * @brief gestisce l'invio dell'AVAILABLE_CARD
 * (l'ho reso una funzione in quanto dev'essere chiamato in porzioni diverse di codice)
 */
void available_card(int sockfd){
    int reg[n_users];
    int j=0;
    for(int i=0;i<MAX_UTENTI&&j<n_users;i++){
        if(ports[i]==0)
            continue;
        reg[j]=ports[i];
        j++;
    }

    //creo il pacchetto
    Packet av_pkt;
    av_pkt.cmd=AVAILABLE_CARD;
    av_pkt.n_users=n_users;

    //creo l'indirizzo generico (senza porta)
    struct sockaddr_in host_addr;
    host_addr.sin_family=AF_INET;
    inet_pton(AF_INET,SERVER_IP,&host_addr.sin_addr);

    //per ogni utente registrato modifico il pacchetto inserendo la lista degli altri utenti registrati, modifico l'indirizzo e invio 
    for(int i=0;i<n_users;i++){
        for(int j=0,J=0;J<n_users;j++,J++){
            if(J==i){
                J++;
            }
            av_pkt.users_ports[j]=reg[J];
        }
        host_addr.sin_port=htons(reg[i]);
        send_packet(sockfd,&av_pkt,&host_addr);   
    }

    //torno al loop in modo da mettermi in ascolto per la ricezione di ACK_CARD e di eventuali CARD_DONE o altro
}

int main(){

    init_lavagna();
    print_lavagna();

    //creo il socket UDP
    int sockfd;
    if(sockfd=socket(AF_INET,SOCK_DGRAM,0)<1){
        perror("Errore nella creazione del socket");
        return 1;
    }

    //assegno un indirizzo al socket
    struct sockaddr_in my_addr;
    my_addr.sin_family=AF_INET;
    inet_pton(AF_INET,SERVER_IP,&my_addr.sin_addr);
    my_addr.sin_port=htons(SERVER_PORT);

    if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(my_addr))<0){
        perror("Errore nella bind");
        close(sockfd);
        return 1;
    }

    printf("Server in ascolto sulla porta %i...\n",SERVER_PORT);

    while(1){
        Packet rcv_pkt;
        struct sockaddr_in host_addr;
        socklen_t host_addr_len;
        recvfrom(sockfd,&rcv_pkt,sizeof(Packet),0,(struct sockaddr*)&host_addr,&host_addr_len);

        switch(rcv_pkt.cmd){

            case(HELLO):{
                //salvo la porta e aggiorno il contatore
                int port=rcv_pkt.sender_port;
                ports[port-USER_START_PORT]=port;
                n_users++;

                printf("Utente porta: %i registrato...\n Numero utenti:%i\n",port,n_users);

                //se ci sono almeno due utenti mando l'AVAILABLE_CARD
                if(n_users >=2){
                    available_card(sockfd);
                }
                break;
            }
                     
            case(QUIT):{
                //rimuovo la porta dall'array e aggiorno il contatore
                int port=rcv_pkt.sender_port;
                ports[port-USER_START_PORT]=0;
                n_users--;

                //sposto un eventuale carta in svolgimento dall'utente in TODO
                if(doing[port-USER_START_PORT]!=-1){
                    move_card(doing[port-USER_START_PORT],TODO);
                    //se ci sono almeno due utenti mando l'available card
                }
                break;
            }
                        
            case(ACK_CARD):{
                //salvo in doing l'id della carta
                doing[rcv_pkt.sender_port-USER_START_PORT]=rcv_pkt.card.id;
                //sposto la carta nella colonna DOING
                move_card(rcv_pkt.card.id,DOING);
                print_lavagna();

                printf("Carta di id: %i assegnata all'utente %i\n",rcv_pkt.card.id,rcv_pkt.sender_port);
                break;
            }
                         
            case(CARD_DONE):{
                //setto a -1 il doing dell'utente
                doing[rcv_pkt.sender_port-USER_START_PORT]=-1;
                //sposto la carta nella colonna DONE
                move_card(rcv_pkt.card.id,DONE);
                print_lavagna();

                printf("Carta di id: %i completata\n",rcv_pkt.card.id);

                //se ci sono almeno 2 utenti chiamo l'AVAILABLE_CARD in quanto l'utente ha terminato una carta
                if(n_users >=2){
                    available_card(sockfd);
                }
                break;
            }
                         
            case(CREATE_CARD):{
                //aggiungo la carta alla lavagna
                add_card(rcv_pkt.card);
                print_lavagna();
                break;
            }
                         
        }
    }


}