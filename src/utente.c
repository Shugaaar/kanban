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



int costs[MAX_UTENTI-1]; //costi ricevuti dagli altri utenti
int peer_ports[MAX_UTENTI-1]; //array che per ogni i indica la porta di colui che ha costs[i], serve solo per scegliere sul numero di porta in caso di parità di costo

int n_peer_remaining; //counter di peer da cui ricevere ancora il costo
int n_peer=0; //numero di peer da contattare dopo AVAILABLE_CARD
int my_cost; //costo calcolato randomicamente

struct Card doing; //se id!=-1 corrisponde alla carta che sto eseguendo



int main(int argc,char* argv[]){
    if(argc !=2){
        fprintf(stderr,"Uso: %s [porta utente]\n",argv[0]);
        return 1;
    }
    int my_port=atoi(argv[1]);
    if(my_port<USER_START_PORT||my_port>USER_START_PORT+MAX_UTENTI){
        fprintf(stderr,"La porta utente dev'essere compresa tra %i e %i\n",USER_START_PORT,USER_START_PORT+MAX_UTENTI);
        return 1;
    }

    //creo il socket di tipo UDP
    int sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd<0){
        perror("Errore durante la creazione del socket\n");
        return 1;
    }

    //assegno un indirizzo al socket
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family=AF_INET;
    my_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    my_addr.sin_port=htons(my_port);

    if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(my_addr))<0){
        perror("Errore nella bind");
        close(sockfd);
        return 1;
    }
    
    //creo l'indirizzo della lavagna
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(SERVER_PORT);
    inet_pton(AF_INET,SERVER_IP,&server_addr.sin_addr); //converto e assegno l'ip al socket della lavagna

    //creo il pacchetto per mandare l'HELLO
    Packet pkt;
    memset(&pkt, 0, sizeof(Packet)); //pulisco i dati occupati dal pacchetto
    pkt.cmd=HELLO;
    pkt.sender_port=my_port;
 
    send_packet(sockfd, &pkt, &server_addr);

    printf("Utente porta %d: Registrato alla lavagna\n", my_port);

    //DEVO RICEVERE DATI SIA DA TASTIERA CHE DAL SOCKET, in modo da ricevere pacchetti e tenere abilitato il terminale 
    //contemporaneamente, per fare ciò mi servo dell'IO MULTIPLEXING

    fd_set fd_list; //creo la lista di file descriptor

    while(1){ //loop principale, attendo che arrivi un pacchetto e agisco di conseguenza

        //uso le macro per gestire la lista di fd
        FD_ZERO(&fd_list);          // pulisco
        FD_SET(sockfd, &fd_list);   // Aggiungo il socket
        FD_SET(STDIN_FILENO, &fd_list); // Aggiungo la tastiera 

        int max_fd = sockfd; //suppongo che il max sia il socket in quanto STDIN vale 0

        if (select(max_fd + 1, &fd_list, NULL, NULL, NULL) < 0) { //mi metto in attesa su entrambi i fronti
            perror("Errore nella select");
            return 1;
        }

        if(FD_ISSET(sockfd,&fd_list)){

            Packet rcv_pkt;
            struct sockaddr_in sender_addr;
            socklen_t addr_len = sizeof(sender_addr);

            //aspetto la ricezione di un pacchetto
            if(recvfrom(sockfd,&rcv_pkt,sizeof(Packet),0,(struct sockaddr*)&sender_addr,&addr_len)<0){
                perror("Errore nella ricezione del pacchetto");
                close(sockfd);
                return 1;
            }

            switch(rcv_pkt.cmd){

                case AVAILABLE_CARD:{
                    printf("Ricevuta card disponibile, id:%i\n",rcv_pkt.card.id);
                
                    copy_card(rcv_pkt.card,&doing);//salvo i dati della carta
                    srand(time(NULL) ^ my_port); //ho aggiunto lo xor con my_port perchè sennò usciva lo steso costo per ogni utente
                    my_cost=rand(); //genero il costo randomico , non so pk ma risulta lo stesso in tutti gli utenti
                    printf("Costo generato: %i\n",my_cost);

                    //creo il pacchetto con il mio costo
                    Packet cost_pkt;
                    memset(&cost_pkt,0,sizeof(Packet));
                    cost_pkt.cmd=CHOOSE_USER;
                    cost_pkt.cost=my_cost;
                    cost_pkt.sender_port=my_port;

                    struct sockaddr_in host_addr;
                    host_addr.sin_family=AF_INET;
                    inet_pton(AF_INET,SERVER_IP,&host_addr.sin_addr);
                    n_peer=rcv_pkt.n_users-1; //aggiorno il numero di peer da gestire
                    n_peer_remaining=n_peer; // inizializzo il counter di costi da ricevere

                    for(int i=0;i<rcv_pkt.n_users-1;i++){ //spedisco il costo ai vari peer
                        host_addr.sin_port=htons(rcv_pkt.users_ports[i]);
                        send_packet(sockfd,&cost_pkt,&host_addr);
                    }
                    break;
                }

                case CHOOSE_USER:{
                    costs[n_peer-n_peer_remaining]=rcv_pkt.cost; //salvo il costo ricevuto
                    printf("Costo ricevuto da %i: %i\n",rcv_pkt.sender_port,rcv_pkt.cost);
                    peer_ports[n_peer-n_peer_remaining]=rcv_pkt.sender_port; //salvo la porta ssociata al costo
                    n_peer_remaining--;

                    if(!n_peer_remaining){ 
                        bool chosen=true;
                        for(int i=0;i<n_peer;i++){ //se il mio costo è maggiore di almeno un peer non sono il chosen
                            if(my_cost>costs[i]){
                                chosen=false;
                                doing.id=-1;
                                break;
                            }
                            if(my_cost==costs[i]){ //se ho il costo uguale a qualcun'altro e ho porta maggiore non posso essere il chosen
                                printf("SONO ENTRATO NEL CASO DI PARITA' DI COSTO\n");
                                if(my_port>peer_ports[i]){
                                    chosen=false;
                                    doing.id=-1;
                                    break;
                                }
                            }
                        }
                        if(chosen){ // se sono il chosen mando alla lavagna l'ACK_CARD

                            printf("Ti è stata assegnata una carta\n");
                            print_card(doing);

                            Packet ack_pkt;
                            memset(&ack_pkt,0,sizeof(Packet));
                            ack_pkt.cmd=ACK_CARD;
                            ack_pkt.sender_port=my_port;
                            copy_card(doing,&ack_pkt.card);
                            send_packet(sockfd,&ack_pkt,&server_addr);

                            sleep(10);

                            //dopo 10 secondi mando il CARD_DONE

                            Packet done_pkt;
                            memset(&done_pkt,0,sizeof(Packet));
                            done_pkt.cmd=CARD_DONE;
                            done_pkt.sender_port=my_port;
                            copy_card(doing,&done_pkt.card);
                            send_packet(sockfd,&done_pkt,&server_addr);
    
                        }
                    }
                    break;
                }
            }             
        }

        if(FD_ISSET(STDIN_FILENO,&fd_list)){
            char cmd[256];
            scanf("%s",cmd); //leggo da tastiera il comando

            if(strcmp(cmd,"CREATE_CARD")==0){
                int id=-1;
                ColumnType col;
                char text[256];

                printf("Creazione di una nuova carta.\nInserire l'Id (usarne uno già esistente annullerà la creazione): ");
                scanf("%i",&id);
                while(id<0||id>=MAX_CARDS){
                    printf("L'id dev'essere compreso tra 0 e %i, riprovare:\n",MAX_CARDS);
                    scanf("%i",&id);
                }
                printf("Inserire il numero associato alla colonna (TODO 0, DONE 2): ");
                scanf("%i",&col);
                while(col<0||col>2){
                    printf("Il numero di colonna dev'essere 0 o 2, riprovare:\n");
                    scanf("%i",&col);
                }
                printf("Inserire il testo dell'attività (Max 256 caratteri):\n");

                //siccome la scanf lascia nello stdin dei caratteri residui, li tolgo in modo che la fget non li prenda
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF); 

                // Ora fgets troverà il buffer pulito e si fermerà per l'input
                if (fgets(text, sizeof(text), stdin)) {
                     text[strcspn(text, "\n")] = 0; // Rimuove il \n finale catturato da fgets
                }

                struct Card c;
                create_card(&c,id,col,text);

                //creo il pacchetto
                Packet cr_pkt;
                memset(&cr_pkt,0,sizeof(Packet));
                cr_pkt.sender_port=my_port;
                cr_pkt.cmd=CREATE_CARD;
                copy_card(c,&cr_pkt.card);

                send_packet(sockfd,&cr_pkt,&server_addr);

            }else if(strcmp(cmd,"QUIT")==0){

                Packet quit_pkt;
                memset(&quit_pkt,0,sizeof(Packet));
                quit_pkt.sender_port=my_port;
                quit_pkt.cmd=QUIT;

                send_packet(sockfd,&quit_pkt,&server_addr);
                return 0;

            }else{
                printf("Comandi validi: CREATE_CARD , QUIT \n");
            }
        }




    }


}