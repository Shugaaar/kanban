# Variabili
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
BIN_DIR = bin
SRC_DIR = src

# Colori per l'output
YELLOW = \033[0;33m
NC = \033[0m

.PHONY: all clean setup help

all: setup $(BIN_DIR)/lavagna $(BIN_DIR)/utente
	@echo "$(YELLOW)Compilazione completata con successo!$(NC)"

# Crea la struttura delle cartelle se non esiste
setup:
	@mkdir -p $(BIN_DIR)

$(BIN_DIR)/lavagna: $(SRC_DIR)/lavagna.c
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/utente: $(SRC_DIR)/utente.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BIN_DIR)
	@echo "Pulizia effettuata."

# Comando help
help:
	@echo "Comandi disponibili:"
	@echo "  make       - Compila tutto il progetto"
	@echo "  make clean - Rimuove i file binari"