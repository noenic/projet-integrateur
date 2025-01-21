#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/shm.h>
#include "lib_interfaces.h" 

#define PORT 12345  // Le port sur lequel le serveur écoute
#define TIMEOUT 5   // Temps d'écoute en secondes
#define BUF_SIZE 64  // Taille du buffer pour recevoir les messages
#define MAX_HOSTS 1024 // Limite des hôtes uniques à gérer

typedef struct {
    char ip[INET_ADDRSTRLEN];
    char hostname[BUF_SIZE];
} HostEntry;

// Structure partagée entre processus
typedef struct {
    HostEntry entries[MAX_HOSTS];
    int count;
} SharedHostTracker;

// Fonction pour vérifier si une adresse est déjà enregistrée
int is_host_tracked(SharedHostTracker *tracker, const char *ip) {
    for (int i = 0; i < tracker->count; i++) {
        if (strcmp(tracker->entries[i].ip, ip) == 0) {
            return 1; // Déjà présent
        }
    }
    return 0; // Pas encore enregistré
}

// Ajouter un hôte à la liste
void add_host_to_tracker(SharedHostTracker *tracker, const char *ip, const char *hostname) {
    if (tracker->count < MAX_HOSTS) {
        strncpy(tracker->entries[tracker->count].ip, ip, INET_ADDRSTRLEN - 1);
        tracker->entries[tracker->count].ip[INET_ADDRSTRLEN - 1] = '\0'; // Sécuriser la fin
        strncpy(tracker->entries[tracker->count].hostname, hostname, BUF_SIZE - 1);
        tracker->entries[tracker->count].hostname[BUF_SIZE - 1] = '\0'; // Sécuriser la fin
        tracker->count++;
    }
}

// Fonction pour rendre le socket non-bloquant
void set_socket_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl() failed");
        exit(1);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl() failed to set non-blocking");
        exit(1);
    }
}

// Fonction pour gérer la connexion d'un client
void handle_client(int client_sockfd, SharedHostTracker *tracker) {
    char buffer[BUF_SIZE];
    char* client_ip = NULL;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    if (getpeername(client_sockfd, (struct sockaddr *)&client_addr, &addr_len) == 0) {
        client_ip = inet_ntoa(client_addr.sin_addr);
    }
    ssize_t recv_len = recv(client_sockfd, buffer, BUF_SIZE - 1, 0);
    if (recv_len < 0) {
        perror("recv() failed");
    } else {
        buffer[recv_len] = '\0'; // Terminer la chaîne reçue
        if (!is_host_tracked(tracker, client_ip)) {
            add_host_to_tracker(tracker, client_ip, buffer);
            printf("%s (%s)\n", buffer, client_ip);
        }
    }
    close(client_sockfd); // Fermer le socket une fois la communication terminée
}

// Fonction pour configurer et écouter en TCP pendant 5 secondes
void listen_tcp_for_5_seconds(SharedHostTracker *tracker) {
    // Utilisation de initialize_socket pour créer un socket
    int sockfd = initialize_socket(SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return;
    }

    if (listen(sockfd, 3) < 0) {
        perror("Listen failed");
        close(sockfd);
        return;
    }

    printf("Écoute sur le port %d pendant %d secondes...\n\n", PORT, TIMEOUT);

    set_socket_nonblocking(sockfd);  // Mettre le socket en mode non-bloquant

    time_t start_time = time(NULL);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);

        if (client_sockfd >= 0) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("Fork failed");
                close(client_sockfd);
            } else if (pid == 0) { // Processus enfant
                close(sockfd); // L'enfant n'a pas besoin du socket serveur
                handle_client(client_sockfd, tracker);
                exit(0);
            } else {
                close(client_sockfd); // Le parent n'a pas besoin du socket client
            }
        }

        if (time(NULL) - start_time >= TIMEOUT) {
            break;
        }

        usleep(100000);
    }

    printf("\nFin du temps d'attente. Fermeture...\n");
    close(sockfd);
}

int main(int argc, char *argv[]) {
    int hops = 1;

    // Parser les arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-hops") == 0 && i + 1 < argc) {
            hops = atoi(argv[i + 1]);
            i++;
        }
    }

    printf("Démarrage de neighborshow (client mode)... Hops: %d\n", hops);

    // Création de la mémoire partagée
    int shm_id = shmget(IPC_PRIVATE, sizeof(SharedHostTracker), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget failed");
        exit(1);
    }

    SharedHostTracker *tracker = (SharedHostTracker *)shmat(shm_id, NULL, 0);
    if (tracker == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    tracker->count = 0;

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0) { // Processus enfant
        listen_tcp_for_5_seconds(tracker);
        exit(0);
    } else { // Processus parent
        usleep(2000);
        struct Interface *interfaces = NULL;
        int if_count = get_interfaces(&interfaces);
        send_broadcast_on_interfaces(interfaces, PORT, if_count, hops, NULL, NULL);

        wait(NULL); // Attendre la fin du processus enfant
    }

    // Libérer la mémoire partagée
    shmdt(tracker);
    shmctl(shm_id, IPC_RMID, NULL);

    return 0;
}
