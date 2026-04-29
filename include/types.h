#ifndef TYPES_H
#define TYPES_H

#include <time.h>
#include "config.h"

/* Header file che si occupa di definire i tipi struct che verranno utilizzati dagli altri moduli,
ho diviso le strutture dalla logica in modo da evitare ridondanze di codice nei vari moduli e 
definire una struttura comune ad essi, utile allo scambio dei dati*/


// Tipo che rappresenta il tipo di colonna, si usa una enum anzichè stringhe per risparmiare spazio e fare confronti più rapidi
typedef enum{
    TODO,
    DOING,
    DONE,
    EMPTY //indica che la carta non è ancora stata creata
} ColumnType;

// Tipo che rappresenta una card
typedef struct{
    int id;
    ColumnType col;  //collocazione della carta
    char text[MAX_TEXT_LEN]; //descrizione attività

    int user_port;  //num di porta dell'utente 
    time_t timestamp; //ts ultima modifica

    Card* next; // puntatore alla prossima carta della coda

} Card;

#endif