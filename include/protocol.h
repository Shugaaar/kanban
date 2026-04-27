#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "types.h"

/* Header file contenente la lista di tutti i comandi utilizzabili da server/client e della struttura dati che definisce
 il formato dell'informazione che vienen scambiato in un pacchetto, ciò rende la comunicazione più semplice e evita 
 di specificare ogni volta la dimensione del messaggio*/

 typedef enum{
    //UTENTE
    CREATE_CARD, //crea una carta
    HELLO,  //registrazione alla lavagna
    QUIT, //uscita dalla lavagna
    PONG_LAVAGNA, //risposta al ping della lavagna
    CHOOSE_USER, //comunica il proprio costo agli altri utenti
    ACK_CARD, //comunica alla lavagna che eseguirà l'attività
    CARD_DONE, //comunica alla lavagna che ha finito l'attività
    //LAVAGNA
    MOVE_CARD, //sposta la card di colonna
    SHOW_LAVAGNA, //mostra lo stato della lavagna
    SEND_USER_LIST, //comunica le porte degli utenti connessi,escluso il destinatario
    PING_USER, //richiede un pong all'utente
    AVAILABLE_CARD //invia a tutti gli utenti la prima carta TODO

 }Command;

 typedef struct {

   Command cmd; //tipo di comando
   int sender_port; //porta del sender

   //UTENTI
   Card card; //card creata con CREATE_CARD
   int cost; //costo dell'utente

   //LAVAGNA
   int n_users; //numero di utenti registrati
   int users_ports[MAX_UTENTI-1]; //lista di porte degli utenti

 } Packet;

#endif