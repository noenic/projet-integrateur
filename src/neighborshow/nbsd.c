#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include "lib_interfaces.h"

#define BUF_SIZE 64  // Taille maximale du message
#define PORT 12345   // Le port sur lequel le serveur écoute


// Variable globale pour savoir si le mode débogage est activé
int debug_enabled = 0;


// Fonction pour afficher un message de débogage, si le mode débogage est activé
void debug(const char *format, ...) {
    if (debug_enabled) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

// Fonction pour vérifier si l'option relay est présente
int is_relay_mode(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--relay") == 0 || strcmp(argv[i], "-r") == 0) {
            printf("Mode relais activé.\n");
            return 1;  // Mode relais activé
        }
    }
    return 0;  // Mode relais désactivé
}

// Fonction pour extraire l'entier N de la chaîne et l'adresse IP
void extract_n_and_ip_from_message(char *message, int *N, char *ip) {
    // La chaîne est de la forme "N@adresse_ip"
    char *token = strtok(message, "@");
    if (token != NULL) {
        *N = atoi(token);  // Mettre à jour N en déférant le pointeur
    }

    token = strtok(NULL, "@");
    if (token != NULL) {
        strncpy(ip, token, 15);  // Limiter à 15 caractères + '\0'
        ip[15] = '\0';  // Assurer la null-termination
    }
}

// Fonction pour gérer la réception des messages
void handle_client(int sockfd, struct sockaddr_in *client_addr, const struct Interface *interfaces, const struct Interface curentIf, int if_count, int relay) {
    char buffer[BUF_SIZE];
    socklen_t client_addr_len = sizeof(*client_addr);

    ssize_t recv_len = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *) client_addr, &client_addr_len);
    if (recv_len < 0) {
        perror("recvfrom");
        return;
    }

    buffer[recv_len] = '\0';  // Terminer la chaîne
    debug("Message reçu de %s:%d : %s sur l'interface %s\n", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port), buffer, curentIf.name);


    int N = 0;
    char ip[16] = {0};  // Initialiser l'IP à une chaîne vide

    // Passer l'adresse de N et de ip
    extract_n_and_ip_from_message(buffer, &N, ip);

    // Problème de réception sur plusieurs interfaces : ignorer les doublons locaux
    struct in_addr client_ip_addr;
    inet_pton(AF_INET, ip, &client_ip_addr);

    for (int i = 0; i < if_count; i++) {
        for (int j = 0; j < interfaces[i].ip4_count; j++) {
            if (strcmp(interfaces[i].ip4_addresses[j], inet_ntoa(client_addr->sin_addr)) == 0) {
                debug("Message ignoré car reçu de notre propre adresse IP (%s)\n", inet_ntoa(client_addr->sin_addr));
                return;
            }
        }
    }

    // Répondre par le hostname
    char *response = get_hostname();

    // Connexion au client via TCP
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd < 0) {
        perror("socket");
        return;
    }

    client_addr->sin_port = htons(PORT);  
    inet_pton(AF_INET, ip, &client_addr->sin_addr);

    debug("Connexion à %s:%d...\n", ip, PORT);
    if (connect(client_sockfd, (struct sockaddr *) client_addr, client_addr_len) < 0) {
        perror("connect");
        close(client_sockfd);
        return;
    }

    // Envoyer la réponse
    if (send(client_sockfd, response, strlen(response), 0) < 0) {
        perror("send");
    } else {
        printf("(%s) -> (%s) [%s]\n", ip, response, curentIf.name);
    }

    close(client_sockfd);

    // Si le mode relais est activé et N > 1
    if (relay && N > 1) {
        N--;
        char new_message[BUF_SIZE];
        snprintf(new_message, BUF_SIZE, "%d@%s", N, ip);
        send_broadcast_on_interfaces(interfaces, PORT, if_count, N, &curentIf, new_message);
        debug("Message relayé : %s\n", new_message);
    }
}

// Fonction pour écouter sur chaque interface et créer un fork pour chaque adresse
void listen_on_interfaces(const struct Interface *interfaces, int if_count, int relay) {
    int sockfd;
    struct sockaddr_in server_addr;
    pid_t pid;

    for (int i = 0; i < if_count; i++) {
        if (interfaces[i].ip4_count == 0 || strcmp(interfaces[i].name, "lo") == 0) {
            continue;
        }

        // Créer un socket UDP
        if ((sockfd = initialize_socket(SOCK_DGRAM, 1)) < 0) {
            return;
        }

        // Configurer l'adresse du serveur pour écouter sur toutes les interfaces
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);  // Port arbitraire
        char *broadcast_address = get_broadcast_address(interfaces[i]);
        server_addr.sin_addr.s_addr = inet_addr(broadcast_address);

        debug("Ecoute sur l'interface %s (%s)...\n", interfaces[i].name, broadcast_address);
        
        // Lier le socket à l'adresse du serveur
        if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
            perror("bind");
            close(sockfd);
            return;
        }

        // Créer un processus enfant pour chaque interface
        if ((pid = fork()) < 0) {
            perror("fork");
            close(sockfd);
            return;
        }
        if (pid == 0) {
            // Processus enfant
            while (1) {
                struct sockaddr_in client_addr;
                handle_client(sockfd, &client_addr, interfaces, interfaces[i], if_count, relay);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    // Gérer l'option -v pour activer le mode débogage
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            debug_enabled = 1;
            break;
        }
    }

    printf("Démarrage de neighborshow (mode serveur)...\n");
    struct Interface *interfaces = NULL;
    int if_count = get_interfaces(&interfaces);  // Récupérer les interfaces
    int relay = is_relay_mode(argc, argv);

    // Lancer l'écoute sur les interfaces
    listen_on_interfaces(interfaces, if_count, relay);

    // Attendre que les processus enfants se terminent
    while (wait(NULL) > 0);
    
    return 0;
}
