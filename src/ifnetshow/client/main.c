#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#define DEFAULT_PORT 7652

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s -n <server_ip_or_hostname> (-i <interface_name> | -a for all)\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sock;
    struct sockaddr_in server_addr;
    size_t size;
    char *info;
    char interface_name[256] = "";  // Nom de l'interface à envoyer (initialisé à chaîne vide)
    char *server_ip = NULL;

    // Traitement des arguments
    // ./client -n <server_ip_or_hostname> (-i <interface_name> | -a for all)

    int interface_specified = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            server_ip = argv[++i];
        } else if (strcmp(argv[i], "-i") == 0) {
            if (i + 1 < argc) {
                strncpy(interface_name, argv[++i], sizeof(interface_name) - 1);
            } else {
                fprintf(stderr, "Erreur: il faut spécifier un nom d'interface après -i\n");
                exit(EXIT_FAILURE);
            }
            interface_specified = 1;
        } else if (strcmp(argv[i], "-a") == 0) {
            interface_specified = 1;
        } else {
            fprintf(stderr, "Erreur: argument invalide: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }

    if (server_ip == NULL) {
        fprintf(stderr, "Erreur: l'adresse du serveur est obligatoire avec -n\n");
        exit(EXIT_FAILURE);
    }

    if (!interface_specified) {
        fprintf(stderr, "Erreur: il faut spécifier soit -i <interface_name> soit -a\n");
        exit(EXIT_FAILURE);
    }

    if (server_ip == NULL) {
        fprintf(stderr, "Erreur: l'adresse du serveur est obligatoire avec -n\n");
        exit(EXIT_FAILURE);
    }

    if (strlen(interface_name) == 0 && strcmp(argv[3], "-a") != 0) {
        fprintf(stderr, "Erreur: il faut spécifier soit -i <interface_name> soit -a\n");
        exit(EXIT_FAILURE);
    }

    // Créer le socket du client
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Erreur de création du socket");
        exit(EXIT_FAILURE);
    }

    // Configurer l'adresse du serveur
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);

    // Convertir l'adresse IP ou le nom d'hôte en adresse binaire
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        struct hostent *host = gethostbyname(server_ip);
        if (host == NULL) {
            perror("Erreur de résolution de nom d'hôte");
            close(sock);
            exit(EXIT_FAILURE);
        }
        server_addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];
    }

    // Se connecter au serveur
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur de connexion");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Envoyer le nom de l'interface ou la demande pour toutes les interfaces
    if (strlen(interface_name) > 0) {
        if (send(sock, interface_name, strlen(interface_name) + 1, 0) < 0) {
            perror("Erreur d'envoi du nom de l'interface");
            close(sock);
            exit(EXIT_FAILURE);
        }
    } else {
        if (send(sock, "all", 4, 0) < 0) {  // "all" pour demander toutes les interfaces
            perror("Erreur d'envoi de la demande d'interface");
            close(sock);
            exit(EXIT_FAILURE);
        }
    }

    // Recevoir la taille des données
    if (recv(sock, &size, sizeof(size), 0) < 0) {
        perror("Erreur de réception de la taille");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Allouer de la mémoire pour recevoir les données
    info = malloc(size);
    if (info == NULL) {
        perror("Erreur d'allocation mémoire");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Recevoir les informations des interfaces
    if (recv(sock, info, size, 0) < 0) {
        perror("Erreur de réception des informations");
        free(info);
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Afficher les informations reçues
    printf("Informations des interfaces réseau du serveur %s:\n", server_ip);
    printf("%s\n", info);

    // Libérer la mémoire allouée
    free(info);

    // Fermer la connexion
    close(sock);
    return 0;
}
