#include "../include/protocol.h"

//funzione generica condivisa tra server e client per inviare un pacchetto
void send_packet(int socket,Packet *p,struct sockaddr_in *dest){
    sendto(socket,p,sizeof(Packet),0,(struct sockaddr*) dest);
}