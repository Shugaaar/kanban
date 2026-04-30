#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../include/types.h"
#include "../include/config.h"
#include "../include/protocol.h"
#include "utils.c"



int costs[MAX_UTENTI-1]; //costi ricevuti dagli altri utenti
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
    inet_pton(AF_INET,SERVER_IP,&my_addr.sin_addr);
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
    pkt.cmd=HELLO;
    pkt.sender_port=my_port;
    send_packet(sockfd,&pkt,&server_addr);
    printf("Utente porta %d: Registrazione alla lavagna...\n", my_port);

    //DEVO CREARE UNA VERIFICA CHE L'HELLO SIA ARRIVATO

    while(1){ //loop principale, attendo che arrivi un pacchetto e agisco di conseguenza

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
                printf("Ricevuta card disponibile, id:%i",rcv_pkt.card.id);
                
                copy_card(rcv_pkt.card,&doing);//salvo i dati della carta
                srand(time(NULL));
                my_cost=rand(); //genero il costo randomico

                //creo il pacchetto con il mio costo
                Packet cost_pkt;
                cost_pkt.cmd=CHOOSE_USER;
                cost_pkt.cost=my_cost;

                struct sockaddr_in host_addr;
                host_addr.sin_family=AF_INET;
                host_addr.sin_addr.s_addr=INADDR_ANY;
                n_peer_remaining=rcv_pkt.n_users; // aggiorno il counter di costi da ricevere

                for(int i=0;i<rcv_pkt.n_users;i++){ //spedisco il costo ai vari peer
                    host_addr.sin_port=rcv_pkt.users_ports[i];
                    send_packet(sockfd,&cost_pkt,&host_addr);
                }
                break;
            }

            case CHOOSE_USER:{
                costs[n_peer-n_peer_remaining]=rcv_pkt.cost; //salvo il costo ricevuto
                n_peer_remaining--;
                int chosen=1;
                if(!n_peer_remaining){ 
                    for(int i=0;i<n_peer;i++){ //se il mio costo è maggiore di almeno un peer non sono il chosen
                        if(my_cost>costs[i]){
                            chosen=0;
                            doing.id=-1;
                        }
                    }
                    if(chosen){ // se sono il chosen mando alla lavagna l'ACK_CARD

                        printf("Ti è stata assegnata una carta\n");
                        print_card(doing);

                        Packet ack_pkt;
                        ack_pkt.cmd=ACK_CARD;
                        send_packet(sockfd,&ack_pkt,&server_addr);

                        //una volta ricevuto il task abilito il terminale all'host in modo che possa segnalare un CARD_DONE o un QUIT alla lavagna

                        while(1){
                            char cmd[256];
                            scanf("Inserire un comando: %s\n",cmd);
                            if(strcmp(cmd,"CARD_DONE")){
                                Packet done_pkt;
                                done_pkt.cmd=CARD_DONE;
                                send_packet(sockfd,&done_pkt,&server_addr);
                                break;

                            }else if(strcmp(cmd,"QUIT")){
                                Packet quit_pkt;
                                quit_pkt.cmd=QUIT;
                                send_packet(sockfd,&quit_pkt,&server_addr);

                                close(sockfd);
                                return 0;
                            }else{
                                printf("Comando non valido...\n");
                            }
                        }        
                    }
                }
                break;
            }
                
        }




    }


}