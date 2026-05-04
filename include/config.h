#ifndef CONFIG_H
#define CONFIG_H

/* Header file contentente i parametri di configurazione del programma,riunirli in un file 
unico aiuta all'eventuale modifica di questi ultimi*/

#define SERVER_IP "127.0.0.1"       
#define SERVER_PORT 5678 // Porta della lavagna 
#define USER_START_PORT 5679 // Porta iniziale per gli utenti
#define MAX_UTENTI 4 //numero massimo di utenti
#define MAX_CARDS 20 //numero massimo di carte in una lavagna

// Tempo dopo il quale una card in "Doing" riceve un PING (90 secondi) 
#define TIMEOUT_PING_SECONDS 10     
// Tempo di attesa per la risposta PONG (30 secondi) 
#define TIMEOUT_PONG_SECONDS 10  
// Tempo dopo il quale un utente decide di scegliere anche se non sono arrivati tutti i costi  
#define TIMEOUT_CHOOSE_USER 20

#define MAX_TEXT_LEN 256 // Lunghezza massima del testo di un'attività 

#define WIDTH_LAVAGNA 60 //spessore in caratteri della lavagna


#endif