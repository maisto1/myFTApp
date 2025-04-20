#ifndef MY_CLIENT_H
#define MY_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 65536 //64 Kb
#define CONFIRMATION_SIZE 100

#include "myFTclient.h"

// Verifica se il file esiste
int valid_file(const char *filename);

// Inizializza il socket per la connessione
int inizialize_socket();

// Configura l'indirizzo del client
void setup_address(struct sockaddr_in *server_addr, const char *ip, int port);

// Invia le opzioni al server
void send_opt(int sockfd, char *f, char *o, int w, int r, int l);

// Invia un file al server
void send_file(int sockfd, char *filename);

// Riceve un file dal server e lo salva localmente
void write_file(int sockfd, char *filename);

// Ottiene la lista di file o directory dal server
void get_list(int sockfd);

int main(int argc, char *argv[]);


#endif