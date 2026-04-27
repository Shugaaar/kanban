#ifndef CONFIG_H
#define CONFIG_H

/* Header file contentente i parametri di configurazione del programma,riunirli in un file 
unico aiuta all'eventuale modifica di questi ultimi*/

#define SERVER_IP "127.0.0.1"       
#define LAVAGNA_PORT 5678 // Porta della lavagna 
#define UTENTE_PORT_START 5679 // Porta iniziale per gli utenti
#define MAX_UTENTI 4 //numero massimo di utenti

// Tempo dopo il quale una card in "Doing" riceve un PING (90 secondi) 
#define TIMEOUT_PING_SECONDS 90     

// Tempo di attesa per la risposta PONG (30 secondi) 
#define TIMEOUT_PONG_SECONDS 30     

#define MAX_TEXT_LEN 256 // Lunghezza massima del testo di un'attività 

#endif