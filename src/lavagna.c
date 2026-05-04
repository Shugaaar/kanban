#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../include/types.h"
#include "../include/config.h"
#include "../include/protocol.h"
#include "utils.c"

int sockfd; //socket
int my_port=SERVER_PORT; //porta di ascolto
struct sockaddr_in my_addr; //indirizzo del server
struct sockaddr_in host_addr; //indirizzo host generico


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
 * @brief istante di inizio del timer per il ping
 */
time_t last_pingpong;
/**
 * @brief indica se siamo in attesa di un pong
 */
bool pinged=false;

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
        l.col[col]=head->next;
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

    //inizializzo l'array di porte
    for(int i=0;i<MAX_UTENTI;i++){
        ports[i]=0;
    }

    //inizializzo il doing a -1, il che indica che gli utenti non stanno eseguendo alcuna attività
    for(int i=0;i<MAX_UTENTI;i++){
        doing[i]=-1;
    }

    //inizializzo l'array di carte della lavagna
    for(int i=0;i<MAX_CARDS;i++){
        l.cards[i].col=EMPTY;
        l.cards[i].next=NULL;
    }

    //creo alcune carte di esempio
    for(int i=0;i<5;i++){
        char text[256];
        sprintf(text,"Qua verrà visualizzato il testo della  attività #%i (sto aggiungendo lunghezza al testo per vedere come si comporta la stampa)\n",i);
        struct Card c;
        create_card(&c,i,TODO,text);
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
    printf("\n\n");

    char s1[]="--     To Do";
    printf("%s",s1);
    for(int i=0;i< (WIDTH_LAVAGNA/3)-(int)sizeof(s1);i++){
        printf(" ");
    }
    printf("--\n\n");

    struct Card* todo=l.col[0];
    while(todo){
        print_card(*todo);
        todo=todo->next;
    }

    char s2[]="--     Doing";
    printf("%s",s2);
    for(int i=0;i< (WIDTH_LAVAGNA/3)-(int)strlen(s2);i++){
        printf(" ");
    }
    printf("--\n\n");

    struct Card* doing=l.col[1];
    while(doing){
        print_card(*doing);
        doing=doing->next;
    }

    char s3[]="--     Done";
    printf("%s",s3);
    for(int i=0;i< (WIDTH_LAVAGNA/3)-(int)strlen(s3);i++){
        printf(" ");
    }
    printf("--\n\n");

    struct Card* done=l.col[2];
    while(done){
        print_card(*done);
        done=done->next;
    }

    for(int i=0;i<(WIDTH_LAVAGNA)+4;i++){
        printf("-");
    }
    printf("\n");

}

/**
 * @brief segnala che è in corso un ciclo di available_card, necessario 
 * a evitare di coinvolgere utenti che si sono registrati mentre la procedura era in corso;
 * 
 * viene settato in AVAILABLE_CARD e resettato in CARD_DONE
 */
bool doing_available=false;

/**
 * @brief gestisce l'invio dell'AVAILABLE_CARD
 * (l'ho reso una funzione in quanto dev'essere chiamato in porzioni diverse di codice)
 */
void available_card(int sockfd){

    //se non ci sono task da svolgere ritorno
    if(l.col[TODO]==NULL){
        printf("Nessun task da svolgere...\n");
        return;
    }
    
    //segnalo l'inizio della procedura available_card
    doing_available=true;

    int reg[n_users];

    for(int i=0,j=0;j<n_users;i++){ //salvo in un array tutti gli utenti registrati
        if(ports[i]==0) //se ha valore 0 non è ancora registrato
            continue;
        reg[j]=ports[i];
        //printf("%i\n",reg[j]);  //debug
        j++;
    }

    //creo il pacchetto
    Packet av_pkt;
    memset(&av_pkt,0,sizeof(Packet)); //pulisco il pacchetto
    av_pkt.cmd=AVAILABLE_CARD;
    av_pkt.n_users=n_users;

    av_pkt.card=*l.col[TODO]; //metto la carta in cima a TODO nel pacchetto

    //per ogni utente registrato modifico il pacchetto inserendo la lista degli altri utenti registrati, modifico l'indirizzo e invio 
    for(int i=0;i<n_users;i++){
        for(int j=0,J=0;J<n_users;j++,J++){
            if(J==i){ //se J corrisponde all'utente a cui devo inviare salta di una posizione nell'array regs
                J++;
                if(J==n_users) //se J ha già superato il num di utenti interrompi il ciclo
                    break;
            }
            av_pkt.users_ports[j]=reg[J];
            //printf("%i\n",av_pkt.users_ports[j]); //debug
        }
        host_addr.sin_port=htons(reg[i]);
        
        send_packet(sockfd,&av_pkt,&host_addr);
    }
}

void ping_user(){
    if(pinged)
        return;
    Packet ping_pkt;
    memset(&ping_pkt,0,sizeof(Packet));
    ping_pkt.cmd=PING_USER;

    host_addr.sin_port=htons(l.col[DOING]->user_port); //assegno all'indirizzo la porta dell'utente che sta svolgendo il task
    send_packet(sockfd,&ping_pkt,&host_addr);
    pinged=true;
}

void quit(int port){
    //rimuovo la porta dall'array e aggiorno il contatore
    ports[port-USER_START_PORT]=0;
    n_users--;

    printf("Utente porta: %i scollegato.\nNumero utenti:%i\n",port,n_users);

    //sposto un eventuale carta in svolgimento dall'utente in TODO
    if(doing[port-USER_START_PORT]!=-1){
        move_card(doing[port-USER_START_PORT],TODO);

        //se ci sono almeno due utenti mando l'available card
        if(n_users>=2){
            available_card(sockfd);
        }
    }
}

int main(){

    init_lavagna();
    print_lavagna();

    //creo il socket UDP
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd<1){
        perror("Errore nella creazione del socket");
        return 1;
    }

    //assegno un indirizzo al socket
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family=AF_INET;
    inet_pton(AF_INET,SERVER_IP,&my_addr.sin_addr);
    my_addr.sin_port=htons(SERVER_PORT);

    if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(my_addr))<0){
        perror("Errore nella bind");
        close(sockfd);
        return 1;
    }

    //creo l'indirizzo host generico (senza porta)
    memset(&host_addr, 0, sizeof(host_addr));
    host_addr.sin_family=AF_INET;
    inet_pton(AF_INET,SERVER_IP,&host_addr.sin_addr);

    printf("Server in ascolto sulla porta %i...\n",SERVER_PORT);

    //uso la select sia per abilitare il terminale, sia per impostare il timeout per il ping pong

    fd_set fd_list; //creo la lista di file descriptor

    while(1){

        //uso le macro per gestire la lista di fd
        FD_ZERO(&fd_list);          // pulisco
        FD_SET(sockfd, &fd_list);   // Aggiungo il socket
        FD_SET(STDIN_FILENO, &fd_list); // Aggiungo la tastiera 

        int max_fd = sockfd; //suppongo che il max sia il socket in quanto STDIN vale 0


        // creo il timer
        struct timeval* timeout=NULL;

        time_t doing_time=time(NULL)-last_pingpong; //tempo passato dall'ultimo ping
        if(l.col[DOING]!=NULL){ //se c'è un task in esecuzione
            if(doing_time>=TIMEOUT_PING_SECONDS){ //se è passato + di TIMEOUT_PING_SECONDS invio il ping (una volta sola)
                ping_user();
                if(doing_time>=TIMEOUT_PING_SECONDS+TIMEOUT_PONG_SECONDS){ // se è passato + di TIMEOUT_PING_SECONDS + TIMEOUT_PONG
                    pinged=false;
                    quit(l.col[DOING]->user_port); //disconnetto l'utente
                }else{ //sennò setto il timeout del select per catturare la scadenza del pong
                    struct timeval tv;
                    timeout=&tv;
                    timeout->tv_sec=TIMEOUT_PING_SECONDS+TIMEOUT_PONG_SECONDS-doing_time;
                    timeout->tv_usec=0;
                }

            }else{
                //assegno il timeout del ping alla select, se scade mentre sono in ascolto gestisco subito il ping
                struct timeval tv;
                timeout=&tv;
                timeout->tv_sec=TIMEOUT_PING_SECONDS-doing_time;
                timeout->tv_usec=0;
            }  
        }


        int res=select(max_fd + 1, &fd_list, NULL, NULL, timeout);

        if ( res< 0) { //mi metto in attesa su entrambi i fronti
            perror("Errore nella select");
            return 1;
        }else if(res==0){ //se scade il timeout alla select 
            
            if(pinged){
                pinged=0;
                quit(l.col[DOING]->user_port); //disconnetto l'utente
            }else{ 
                ping_user();
            }   
        }


        if(res&&FD_ISSET(sockfd,&fd_list)){ //se è arrivato un pacchetto

            Packet rcv_pkt;
            struct sockaddr_in host_addr;
            memset(&host_addr, 0, sizeof(host_addr));
        
            socklen_t host_addr_len;
            recvfrom(sockfd,&rcv_pkt,sizeof(Packet),0,(struct sockaddr*)&host_addr,&host_addr_len);

            switch(rcv_pkt.cmd){

                case(HELLO):{
                    //salvo la porta e aggiorno il contatore
                    int port=rcv_pkt.sender_port;
                    ports[port-USER_START_PORT]=port;
                    n_users++;

                    printf("Utente porta: %i registrato.\nNumero utenti:%i\n",port,n_users);

                    //se ci sono almeno due utenti e non è già in corso un available_card mando l'AVAILABLE_CARD
                    if(n_users >=2&&doing_available==false){
                    available_card(sockfd);
                    }
                    break;
                }
                     
                case(QUIT):{
                    quit(rcv_pkt.sender_port);
                    break;
                }
                        
                case(ACK_CARD):{
                    
                    //salvo in doing l'id della carta
                    doing[rcv_pkt.sender_port-USER_START_PORT]=rcv_pkt.card.id;
                    //sposto la carta nella colonna DOING
                    move_card(rcv_pkt.card.id,DOING);
                    //aggiorno l'utente associato alla carta
                    l.cards[rcv_pkt.card.id].user_port=rcv_pkt.sender_port;
                    //aggiorno l'istante di inizio del timer
                    last_pingpong=time(NULL);

                    print_lavagna();

                    printf("Carta di id: %i assegnata all'utente %i\n",rcv_pkt.card.id,rcv_pkt.sender_port);
                    break;
                }
                         
                case(CARD_DONE):{
                    if(!l.col[DOING]) // se l'utente manda l'ACK_CARD dopo essere stato catalogato come scollegato questa viene ignorata
                        break;
                    //Se il card done arriva prima del pong,interpreto comunque che l'utente non si è disconnesso
                    last_pingpong=time(NULL);
                    pinged=0;

                    //disabilito la procedura available_card
                    doing_available=false;

                    //setto a -1 il doing dell'utente
                    doing[rcv_pkt.sender_port-USER_START_PORT]=-1;

                    printf("Carta di id: %i completata\n",rcv_pkt.card.id);

                    //sposto la carta nella colonna DONE
                    move_card(rcv_pkt.card.id,DONE);
                    print_lavagna();

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

                    //se ci sono almeno due utenti e non è già in corso un available_card mando l'AVAILABLE_CARD
                    if(n_users >=2&&doing_available==false){
                    available_card(sockfd);
                    }
                    break;
                }

                case(PONG_LAVAGNA):{
                    last_pingpong=time(NULL);
                    pinged=0;
                    if(l.col[DOING])
                        printf("L'utente %i ha risposto al ping\n",l.col[DOING]->user_port);
                    break;
                }
                         
            }

        }

        if(res&&FD_ISSET(STDIN_FILENO,&fd_list)){ //se è arrivato qualcosa da tastiera
            char cmd[256];
            scanf("%s",cmd);
            if(strcmp(cmd,"SHOW_LAVAGNA")==0){
                print_lavagna();

            }else{
                printf("Comandi validi: SHOW_LAVAGNA \n");
            }
        }

        
    }


}