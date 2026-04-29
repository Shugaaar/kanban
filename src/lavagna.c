#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../include/types.h"
#include "../include/config.h"
#include "../include/protocol.h"
#include "utils.c"



int ports[MAX_UTENTI]; //vettore che contiene le porte degli utenti registrati

// Tipo che rappresenta la lavagna
typedef struct{
    int id;
    /* vettore di 3 puntatori a card, implementa 3 code (TODO,DOING,DONE) di card, utile per evitare di allocare spazio non 
    necessario e gestire comodamente l'ordine di arrivo */
    Card* col[3]; 

    Card cards[MAX_CARDS]; //vettore di Card che contiene tutte le card della lavagna


} Lavagna;

Lavagna l;

/**
 * @brief agiunge una carta alla lavagna, verifica che la carta non esista già
 * @param carta da aggiungere
 * @return 0 se la carta esiste già, 1 altrimenti
 */
bool add_card(Card carta){
    Card* c=&l.cards[carta.id];
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
    Card* head=l.col[col];
    Card* c=&l.cards[id];
    if(head->id==id){
        head=NULL;
        return;
    }
    Card* prev;
    Card* cur;
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
    Card* c=&l.cards[id];

    if(c->col==EMPTY)   
        return 0;
    //estraggo la carta dalla precedente coda
    extract_card(id,c->col);
    //aggiorno la testa della nuova coda
    c->next=l.col[col]; 
    l.col[col]=c;
    //aggiorno la carta
    c->col=col;
    c->timestamp=Time(NULL);

    return 1;
}

/**
 * @brief inizializza la lavagna
 */
void init_lavagna(){
    l.id=1;
    l.col[0]=l.col[1]=l.col[2]=0xFFFFFFFF;

    //creo alcune carte di esempio
    for(int i=0;i<5;i++){
        char text[]="attività #"+i;

        Card c;
        create_card(&c,i,0,text);
        add_card(c);
    }

}

int main(){

    init_lavagna();
    print_lavagna(l);


}