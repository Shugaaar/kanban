#ifndef TYPES_H
#define TYPES_H

#include <time.h>

/* Header file che si occupa di definire i tipi struct che verranno utilizzati dagli altri moduli,
ho diviso le strutture dalla logica in modo da evitare ridondanze di codice nei vari moduli e 
definire una struttura comune ad essi, utile allo scambio dei dati*/


// Tipo che rappresenta il tipo di colonna, si usa una enum anzichè stringhe per risparmiare spazio e fare confronti più rapidi
typedef enum{
    TODO,
    DOING,
    DONE
} ColumnType;

// Tipo che rappresenta una card
typedef struct{
    int id;
    ColumnType col;  //collocazione della carta
    char text[256]; //descrizione attività

    int user_port;  //num di porta dell'utente 
    time_t timestamp; //ts ultima modifica

    Card* next; // puntatore alla prossima carta della coda

} Card;

// Tipo che rappresenta la lavagna
typedef struct{
    int id;
    /* vettore di 3 puntatori a card, implementa 3 code (TODO,DOING,DONE) di card, utile per evitare di allocare spazio non 
    necessario e gestire comodamente l'ordine di arrivo */
    Card* col[3]; 

    Card all[10]; //vettore di Card che contiene tutte le card della lavagna


} Lavagna;

#endif