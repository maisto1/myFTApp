#include "myFTserver.h"

void *thread_handler(void *arg) {
    // Estrai i parametri passati al thread
    thread_args_t *args = (thread_args_t *)arg;
    handler_client(args->new_sock, args->dir);
    close(args->new_sock);
    free(args); // Libera la memoria allocata per i parametri del thread
    pthread_exit(NULL);
}

void lock_file(int fd, int type) {
    struct flock fl;
    fl.l_type = type;  // F_RDLCK, F_WRLCK, F_UNLCK
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    // Lock del file
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("[-]Error in locking file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("[DEBUG] File descriptor %d locked successfully with type %d\n", fd, type);
}

void unlock_file(int fd) {
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    // Unlock del file
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perror("[-]Error in unlocking file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("[DEBUG] File descriptor %d unlocked successfully\n", fd);
}


int inizialize_socket(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created\n");    

    return sockfd;
}

void setup_address(struct sockaddr_in *server_addr, const char *ip, int port) {
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    server_addr->sin_addr.s_addr = inet_addr(ip);
}

void check_dir(char *dir){
    // Funzione che mi permette di verificare l'esistenza di una directory (se non esiste viene creata)
    struct stat st = {0};
    if (stat(dir, &st) == -1) {
        if (mkdir(dir, 0700) == -1) {
            perror("[-]Error in creating directory");
            exit(EXIT_FAILURE);
        }
    }
}

char* get_opt(int sockfd) {

    static char options[CONFIRMATION_SIZE] = {0};

    printf("[+]Getting options...\n");

    // Leggo le opzioni inviate dal client
    int bytes_read = recv(sockfd, options, CONFIRMATION_SIZE, 0);
    if (bytes_read <= 0) {
        perror("[-]Error in receiving options!");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    options[bytes_read] = '\0';

    // Invia la conferma al client
    const char *confirmation = "Options received!";
    send(sockfd, confirmation, strlen(confirmation), 0);

    printf("[+]Options retrieved successfully!\n");

    return options;
}

void get_directory(char *p, char *buffer, size_t *buffer_pos, int k ){

    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;

    // Apro la directory
    if((dir = opendir(p)) == NULL){
        snprintf(buffer + *buffer_pos, BUFFER_SIZE - *buffer_pos, "[-]Error opening directory: %s\n", p);
        *buffer_pos += strlen(buffer + *buffer_pos);
        return;
    }

    // Con il *While* leggo i file/dir all'interno della directory corrente (prima iterazione sarebbe *p*) 
    while((entry = readdir(dir)) != NULL){
        char path[BUFFER_SIZE];
        snprintf(path, sizeof(path), "%s/%s", p, entry->d_name); // Costruisco il path
    
        // Ottengo informazioni riguardo il path (se è directory, file, ecc)
        if (stat(path, &statbuf) == -1){
            perror("[-]Stat");
            continue;
        }

        /*Ignoro "." e ".." che sarebbero voci speciali che indicano rispettivamente
        la directory attuale e la directory padre*/
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Aggiungo l'identazione al buffer in questo modo da dare una rappresentazione gerarchica
        for (int i = 0; i < k; i++) {
            *buffer_pos += snprintf(buffer + *buffer_pos, BUFFER_SIZE - *buffer_pos, "     ");
        }

        // Se l'elemento è una directory aggiungi ">"
        if(S_ISDIR(statbuf.st_mode)){
            *buffer_pos += snprintf(buffer + *buffer_pos, BUFFER_SIZE - *buffer_pos, ">%s\n", entry->d_name);

            // Continua ad esplorare ricorsivamente le altre directory (dato che ci troviamo in un directory)
            get_directory(path, buffer, buffer_pos, k + 1);
        } else {

            // Se l'elemento è un file aggiungi "-"
            *buffer_pos += snprintf(buffer + *buffer_pos, BUFFER_SIZE - *buffer_pos, "-%s\n", entry->d_name);
        }
    }

    closedir(dir);
}

int isRelative(const char *path) {
    // Funzione che mi permette di controllare se l'utente sta usando un path relativo per -l -w o -r
    if (strstr(path, "..") != NULL) {
        return 1;
    }
    return 0;
}

void write_file(int sockfd, char *path) {
    FILE *file;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int success = 1; // flag usato per verificae se il file è stato ricevuto correttamente

    printf("[+]Saving file in [%s]...\n", path);

    // Apro il file in modalità scrittura
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);  // O_TRUNC per truncare il file se esiste
    if (fd < 0) {
        perror("[-]Error in opening file");
        const char *error_msg = "[-]Error: File not found\n";
        send(sockfd, error_msg, strlen(error_msg), 0); // Invia un messaggio di errore al client
        close(sockfd);
        return;
    }

    // Lock per la scrittura
    lock_file(fd, F_WRLCK);

    file = fdopen(fd, "wb");
    if (file == NULL) {
        perror("[-]Error in fdopen");
        unlock_file(fd); // Sblocca il file prima di chiuderlo
        close(fd); // Chiudi il descrittore del file
        close(sockfd);
        return;
    }

    // Scrivo i byte ricevuti dal client nel file
    while ((bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        printf("[DEBUG] Received %ld bytes\n", (long)bytes_read);
        fflush(stdout);
        if (fwrite(buffer, 1, bytes_read, file) < (size_t)bytes_read) {
            perror("[-]Error in writing file (possible disk space issue)");
            unlock_file(fd);
            fclose(file);
            success = 0;
            break;
        }
    }

    if (bytes_read < 0) {
        perror("[-]Error in receiving file");
        success = 0;
    }

    
    unlock_file(fd); // Unlock del file
    fclose(file); // Chiudo il file (chiude anche il file descripter)


    const char *response = success ? "[+]File saved successfully!\n" : "[-]Error in saving file!\n"; // gestisco i log in base al flag success
    send(sockfd, response, strlen(response), 0); // invio risposta al client
    printf("%s", response);

    close(sockfd); // chiudo la connessione con il client
}


void send_file(int sockfd, char *path) {
    FILE *file;
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read;

    printf("[+]Sending [%s] to client...\n", path);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("[-]Error in opening file");
        close(sockfd);
        return;
    }

    // Lock per la lettura
    lock_file(fd, F_RDLCK);

    file = fdopen(fd, "rb");
    if (file == NULL) {
        perror("[-]Error in fdopen");
        unlock_file(fd);
        fclose(file);
        close(sockfd);
        return;
    }

    // Invio il file al client
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        printf("[DEBUG] Sending %d bytes\n", bytes_read);
        if (send(sockfd, buffer, bytes_read, 0) < 0) {
            perror("[-]Error in sending file");
            unlock_file(fd);
            fclose(file);
            close(sockfd);
            return;
        }
    }

    shutdown(sockfd, SHUT_WR);

    unlock_file(fd); // Unlock del file
    fclose(file); // Chiudo il file (chiude anche il file descripter)

    int valread = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("%s\n", buffer);
    } else {
        perror("[-]Error in getting client response");
    }

    close(sockfd); // chiudo la connessione con il client
}

void send_list(int sockfd, char *path){

    printf("[+]Sending list to Client...\n");

    size_t buffer_pos = 0;
    int n;
    
    // Alloco memoria al buffer
    char *buffer = malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        perror("malloc");
        return;
    }

    get_directory(path, buffer, &buffer_pos, 0);

    // Leggo il file e lo invio al client
    n = send(sockfd, buffer, BUFFER_SIZE, 0);
    if(n < 0){
        perror("[-]Error in sending list");
        return;
    }

    
    if (strncmp(buffer, "[-]Error opening directory:", 27) == 0) {
        printf("%s\n", buffer);
    } else {
        printf("[+]List sended successfully to Client!\n");
    }

    // Libero il buffer
    free(buffer);

    close(sockfd); // chiudo la connessione con il client
}


void handler_client(int sockfd, char *dir){

    char path[256];

    char *opt = get_opt(sockfd); // Ottengo le opzioni inviate dal client

    // Unpack delle opzioni (divisore --> ',')
    char route[100];
    int w, r, l;

    // Uso sscanf per fare il parsing della stringa delle opzioni
    sscanf(opt, "%[^,],%d,%d,%d", route, &w, &r, &l);

    sprintf(path, "%s/%s", dir, route);

    if (isRelative(route)) {
        const char *error_msg = "[-]Error: Relative paths with '..' are not allowed\n";
        send(sockfd, error_msg, strlen(error_msg), 0);
        printf("[-] Relative path with '..' detected and blocked: %s\n", route);
        close(sockfd);
        return;
    }

    sleep(1);  // aiuta la sincronizzazione

    if(w){
        write_file(sockfd,path);
    }

    if(r){
        send_file(sockfd,path);
    }

    if(l){
        send_list(sockfd, path);
    }

    close(sockfd); // chiudo la connessione con il client
}


int main(int argc, char *argv[]){

    // Inizializzo le opzioni
    char *ip = NULL, *dir = NULL;
    int port = 0;
    
    int opt;
    while ((opt = getopt(argc, argv, "a:p:d:")) != -1) {
        switch (opt) {
        case 'a':
            ip = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'd':
            dir = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s [-a ip_address] [-p port] [-d ft_root_directory]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Verifico che le opzioni non siano NULL
    if(ip == NULL || port == 0 || dir == NULL){
        fprintf(stderr, "Error: Missing required arguments\n"); 
        fprintf(stderr, "Usage: %s [-a ip_address] [-p port] [-d ft_root_directory]\n", argv[0]);
        exit(1);      
    }

    // Funzione che permette di controllare l'esistenza di una directory
    // nel caso contrario verrà creata
    check_dir(dir);

    // Variabili che permettono di inizializzare il server
    int sockfd, new_sock;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    int e;

    printf("[+]Starting Server...\n");
    printf("[+]IP address: %s\n", ip);
    printf("[+]Port: %d\n", port);
    //printf("[+]Root Directory: %s\n", dir);

    // Inizializzo il socket
    sockfd = inizialize_socket();

    // Imposto l'indirizzo del server
    setup_address(&server_addr,ip,port);

    // Associo socket all'address del server
    e = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (e < 0) {
        perror("[-]Error in binding");
        exit(1);
    }

    printf("[+]Binding successfully!\n");

    // Metto il server in ascolto (max 10 connessioni comtemporanee)
    e = listen(sockfd, 10);
    if (e == 0) {
        printf("[+]Listening...\n");
    } else {
        perror("[-]Error in listening");
        exit(1);
    }

    // Attraverso un while infinito permetto al server di rimanere in ascolto
    while (1) {
        addr_size = sizeof(new_addr);
        new_sock = accept(sockfd, (struct sockaddr *)&new_addr, &addr_size); // Accetta la connessione del client
        if (new_sock < 0) {
            perror("[-]Error in accepting connection");
            continue;
        }
        printf("[+]Connection accepted from %s:%d\n", inet_ntoa(new_addr.sin_addr), ntohs(new_addr.sin_port));

        // Alloca memoria per i parametri del thread
        thread_args_t *args = malloc(sizeof(thread_args_t));
        args->new_sock = new_sock;
        args->dir = dir;

        // Crea un thread per gestire la connessione
        pthread_t tid;
        if (pthread_create(&tid, NULL, thread_handler, (void *)args) != 0) {
            perror("[-]Error in creating thread");
            free(args); // Libera la memoria se il thread non può essere creato
        }

        // Stacca il thread per permettere al sistema di liberare le risorse automaticamente
        pthread_detach(tid);
    }

    close(sockfd); // Chiudo la connessione (anche se il ciclo è infinito)
    return 0;
}
