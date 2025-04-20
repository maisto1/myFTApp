#include "myFTclient.h"

int valid_file(const char *filename) {
    // Verifica se il file esiste
    if (access(filename, F_OK) == 0) {
        return 1; // Il file esiste
    } else {
        return 0; // Il file non esiste
    }
}

int inizialize_socket(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("[-]Error in inizialize socket");
        exit(1);
    }
    printf("[+]Client socket created\n");
    return sockfd;
}

void setup_address(struct sockaddr_in *server_addr, const char *ip, int port) {
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    server_addr->sin_addr.s_addr = inet_addr(ip);
}

void send_opt(int sockfd, char *f, char *o, int w, int r, int l){

    char buffer[CONFIRMATION_SIZE] = {0};
    char response[CONFIRMATION_SIZE];
    int n;

    printf("[+]Sending options...\n");

    // Compongo il messaggio da inviare al server
    if(!o || r){
        snprintf(buffer, sizeof(buffer), "%s,%d,%d,%d", f, w, r, l);
    } else {
        snprintf(buffer, sizeof(buffer), "%s,%d,%d,%d",o, w, r, l);
    }

    // Invio delle opzioni
    send(sockfd, buffer, strlen(buffer), 0);

    // Attendo conferma dal server
    int bytes_received = recv(sockfd, response, CONFIRMATION_SIZE, 0);
    if (bytes_received <= 0) {
            perror("[-]Error in recieving Server response");
            close(sockfd);
            exit(EXIT_FAILURE);
    }

    response[bytes_received] = '\0';
    printf("[+]Server response: %s\n", response);
}


void send_file(int sockfd, char *filename){
    FILE *file;
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read;

    printf("[+]Sending [%s] to server...\n", filename);

    // Apro il file da inviare al server
    file = fopen(filename, "rb");
    if (file == NULL) {
        perror("[-]Error in opening file!");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Invio il file al server in pacchetti
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        printf("[DEBUG] Sending %d bytes\n", bytes_read);
        if (send(sockfd, buffer, bytes_read, 0) < 0) {
            perror("[-]Error in sending file to server!");
            fclose(file);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }

    // Segnala la fine della trasmissione al server
    shutdown(sockfd, SHUT_WR);

    fclose(file);

    // Aspetto la risposta dal server
    int valread = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("%s\n", buffer);
    } else {
        perror("[-]Error in getting server response");
    }

    // Chiudo la connessione con il server
    close(sockfd);
}

void write_file(int sockfd, char *filename) {
    FILE *file;
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read;
    int success = 1; // flag usato per verificae se il file è stato ricevuto correttamente

    // Ricevo il primo messaggio dal server
    bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes_read <= 0) {
        perror("[-]Error in receiving data");
        close(sockfd);
        return;
    }

    // Controllo se il server ha inviato "FILE NOT FOUND"
    buffer[bytes_read] = '\0';
    if (strcmp(buffer, "FILE NOT FOUND") == 0) {
        printf("[-]Server error: File not found\n");
        close(sockfd);
        return;
    }

    // Se il file esiste, apro il file in modalità scrittura
    printf("[+]Saving file in [%s]...\n", filename);
    file = fopen(filename, "wb");
    if (file == NULL) {
        perror("[-]Error in opening file!");
        close(sockfd);
        return;
    }

    // Continuo a ricevere il file dal server e lo salvo
    do {
        if (fwrite(buffer, 1, bytes_read, file) < (size_t)bytes_read) {
            fclose(file);
            success = 0;
            break;
        }
    } while ((bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0);

    if (bytes_read < 0) {
        perror("[-]Error in receiving file");
        success = 0;
    }

    fclose(file); // chiudo il file 


    const char *response = success ? "[+]File saved successfully!\n" : "[-]Error in saving file!\n"; // gestisco i log in base al flag success
    send(sockfd, response, strlen(response), 0); // Invio risposta al server
    printf("%s", response);

    close(sockfd); // chiudo la connessione con il server
}


void get_list(int sockfd){

    printf("[+]Getting list from Server...\n");

    int n;
    char buffer[BUFFER_SIZE] = {0};

    n = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if(n < 0){
        perror("[-]Error in getting list!");
        exit(1);
    }

    // Controlla se il messaggio ricevuto è un errore
    if (strstr(buffer, "Error") != NULL) {
        printf("%s\n", buffer);
    } else {
        // Mostra la lista ricevuta
        printf("[+]List retrieved successfully from Server!\n");
        printf("-----------------------------\n");
        printf("%s", buffer);
        printf("-----------------------------\n");
    }
    
    memset(buffer, 0, BUFFER_SIZE); // Libero il buffer

    close(sockfd); // chiudo la connessione con il server
}


int main(int argc, char *argv[]) {
    int w = 0, r = 0, l = 0;
    char *a = NULL, *f = NULL, *o = NULL;
    int p = -1;
    int opt;

    while ((opt = getopt(argc, argv, "a:p:f:o:wrl")) != -1) {
        switch (opt) {
            case 'a':
                a = optarg;
                break;
            case 'p':
                p = atoi(optarg);
                break;
            case 'f':
                f = optarg;
                break;
            case 'o':
                o = optarg;
                break;
            case 'w':
                w = 1;
                break;
            case 'r':
                r = 1;
                break;
            case 'l':
                l = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s -w -r -l -a <ip> -p <port> -f <dir_f> -o <dir_o>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Controllo che i flag siano impostati correttamente
    if ((w + r + l) != 1) {
        printf("Errore: Uno e soltanto uno dei flag -w, -r, or -l deve essere impostato.\n");
        return 1;
    }

    // Controllo che i parametri obbligatori siano passati
    if (a == NULL || f == NULL || p == -1) {
        printf("Errore: Argomenti -a, -p, e -f sono obbligatori.\n");
        return 1;
    }

    // Controllo se il file da inviare al server esiste
    if(!valid_file(f) && w){
        perror("[-]Error");
        exit(1);
    }

    // Inizializzo le variabili che riguardano la connessione al server
    int sockfd,e,n;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;

    printf("[+]Starting Client...\n");
    printf("[+]IP: %s\n", a);
    printf("[+]Port: %d\n", p);

    // Inizializzo il Socket
    sockfd = inizialize_socket();

    // Imposto l'indirizzo del server
    setup_address(&server_addr,a,p);

    // Connessione al server
    e = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(e == -1){
        perror("[-]Error in connecting");
        exit(1);
    }
    printf("[+]Connected to server!\n");

    send_opt(sockfd,f,o,w,r,l);

    sleep(1);  // aiuta la sincronizzazione

    if(w){
        send_file(sockfd,f);    // invia file al server
    }

    if(r){
        write_file(sockfd, o == NULL ? f : o); // riceve un file dal server
    }

    if(l){
        get_list(sockfd); // riceve dal server la lista di tutti i file o directory della directory passata nell'argomento -f
    }
}