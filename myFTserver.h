#ifndef MY_SERVER_H
#define MY_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>

#define BUFFER_SIZE 65536 // 64 KB
#define CONFIRMATION_SIZE 100

// Struttura per passare i parametri al thread
typedef struct {
    int new_sock;
    char *dir;
} thread_args_t;

// Gestisce i thread
void *thread_handler(void *arg);

// Blocca lettura o scrittura di un file
void lock_file(int fd, int type);

// Sblocca un file
void unlock_file(int fd);

// Inizializza il socket per la connessione
int inizialize_socket();

// Configura l'indirizzo del server
void setup_address(struct sockaddr_in *server_addr, const char *ip, int port);

// Controlla se una directory esiste, altrimenti  la crea
void check_dir(char *dir);

// Riceve le opzioni dal client
char* get_opt(int sockfd);

// Esplora ricorsivamente una directory inserendo in un buffer file e directory che incontra
void get_directory(char *p, char *buffer, size_t *buffer_pos, int k);

// Controlla se il client sta utilizzando un path relativo
int isRelative(const char *path);

// Riceve un file dal client e lo salva localmente
void write_file(int sockfd, char *path);

// Invia un file al client
void send_file(int sockfd, char *path);

// Invia la lista precedentemente ottunta con 'get_directory()' al client
void send_list(int sockfd, char *path);

// Gestisce le richieste del client
void handler_client(int sockfd, char *dir);

int main(int argc, char *argv[]);


#endif