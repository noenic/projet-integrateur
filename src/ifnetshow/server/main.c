#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "lib_interfaces.h"

#define PORT 7652

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    // Créer le socket serveur
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Erreur de création du socket");
        exit(EXIT_FAILURE);
    }

    // Activer SO_REUSEADDR pour permettre la réutilisation de la socket
    int reuse = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Erreur de setsockopt(SO_REUSEADDR)");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Configurer l'adresse du serveur
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Lier le socket à l'adresse
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur de bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Mettre le serveur en mode écoute
    if (listen(server_sock, 5) < 0) {
        perror("Erreur d'écoute");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Serveur prêt, en attente de connexion sur le port %d\n", PORT);
    while (1) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0) {
            perror("Erreur d'acceptation");
            continue;
        }

        // Créer un processus enfant pour gérer la connexion
        pid_t pid = fork();
        if (pid < 0) {
            perror("Erreur de fork");
            close(client_sock);
            continue;
        }

        if (pid == 0) {
            // Processus enfant
            close(server_sock);
            printf("Connexion acceptée de %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            
            // Récupérer les informations des interfaces
            struct Interface *interfaces = NULL;
            int count = get_interfaces(&interfaces);
            if (count < 0) {
                perror("Erreur de récupération des interfaces");
                close(client_sock);
                exit(EXIT_FAILURE);
            }

            // Lire le nom de l'interface demandé par le client
            char buffer[256];
            int recv_len = recv(client_sock, buffer, sizeof(buffer), 0);
            if (recv_len < 0) {
                perror("Erreur de réception du nom de l'interface");
                close(client_sock);
                exit(EXIT_FAILURE);
            }

            // Si "all" est demandé, envoyer toutes les interfaces
            if (strcmp(buffer, "all") == 0) {
                // Générer la chaîne de caractères contenant les informations des interfaces
                size_t size;
                char *info = interfaces_to_string(interfaces, count, NULL, &size);

                // Envoyer la taille de la chaîne
                if (send(client_sock, &size, sizeof(size), 0) < 0) {
                    perror("Erreur d'envoi de la taille");
                    close(client_sock);
                    free_interfaces(interfaces, count);
                    exit(EXIT_FAILURE);
                }

                // Envoyer les informations des interfaces
                if (send(client_sock, info, size, 0) < 0) {
                    perror("Erreur d'envoi des informations");
                    close(client_sock);
                    free_interfaces(interfaces, count);
                    exit(EXIT_FAILURE);
                }

                free(info);
            } else {
                // Si une interface spécifique est demandée, chercher et envoyer ses informations
                int found = 0;
                for (int i = 0; i < count; i++) {
                    if (strcmp(interfaces[i].name, buffer) == 0) {
                        // Générer les informations pour l'interface trouvée
                        size_t size;
                        char *info = interface_to_string(&interfaces[i], &size);

                        // Envoyer la taille de la chaîne
                        if (send(client_sock, &size, sizeof(size), 0) < 0) {
                            perror("Erreur d'envoi de la taille");
                            close(client_sock);
                            free_interfaces(interfaces, count);
                            exit(EXIT_FAILURE);
                        }

                        // Envoyer les informations de l'interface
                        if (send(client_sock, info, size, 0) < 0) {
                            perror("Erreur d'envoi des informations");
                            close(client_sock);
                            free_interfaces(interfaces, count);
                            exit(EXIT_FAILURE);
                        }

                        free(info);
                        found = 1;
                        break;
                    }
                }

                if (!found) {
                    // Si l'interface n'a pas été trouvée, envoyer un message approprié
                    char *msg = "Aucune interface trouvée";
                    size_t size = strlen(msg) + 1;

                    if (send(client_sock, &size, sizeof(size), 0) < 0) {
                        perror("Erreur d'envoi de la taille");
                        close(client_sock);
                        free_interfaces(interfaces, count);
                        exit(EXIT_FAILURE);
                    }

                    if (send(client_sock, msg, size, 0) < 0) {
                        perror("Erreur d'envoi du message");
                        close(client_sock);
                        free_interfaces(interfaces, count);
                        exit(EXIT_FAILURE);
                    }
                }
            }

            // Libérer la mémoire allouée
            free_interfaces(interfaces, count);

            // Fermer la connexion avec le client
            close(client_sock);
            exit(0);
        } else {
            // Processus parent
            close(client_sock);
        }
    }
}
