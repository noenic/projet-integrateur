// iflib/lib_interfaces.c

#include "lib_interfaces.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <stdarg.h>

/**
 * @brief initialise un socket
 * @param type type de socket (SOCK_DGRAM, SOCK_STREAM, etc.)
 * @param broadcast 1 si le socket doit être configuré pour le broadcast, 0 sinon
 */

int initialize_socket(int type, int broadcast)
{
    int sock;
    if ((sock = socket(AF_INET, type, 0)) < 0)
    {
        perror("Erreur de création du socket");
        exit(EXIT_FAILURE);
    }

    // On autorise le SO_REUSEADDR pour éviter les erreurs "Address already in use"
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("Erreur de configuration du socket pour SO_REUSEADDR");
        exit(EXIT_FAILURE);
    }

    if (broadcast)
    {
        int broadcastEnable = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
        {
            perror("Erreur de configuration du socket pour le broadcast");
            exit(EXIT_FAILURE);
        }
    }
    return sock;
}

/**
 * @brief Ajoute une adresse IP à un tableau d'adresses IP.
 */
void add_ip_address(char ***addresses, int *count, const char *address)
{
    *addresses = realloc(*addresses, (*count + 1) * sizeof(char *));
    (*addresses)[*count] = strdup(address);
    (*count)++;
}

/**
 * @brief Convertit un masque de sous-réseau IPv4 en notation CIDR.
 * @param netmask Masque de sous-réseau IPv4 (ex: 255.255.255.0 -> 24)
 */
int ipv4_to_cidr(const char *netmask)
{
    struct sockaddr_in sa;
    if (inet_pton(AF_INET, netmask, &sa.sin_addr) == 1)
    {
        uint32_t netmask = ntohl(sa.sin_addr.s_addr);
        int cidr = 0;
        while (netmask)
        {
            cidr++;
            netmask <<= 1;
        }
        return cidr;
    }
    return -1;
}

/**
 * @brief Convertit un masque de sous-réseau IPv6 en notation CIDR.
 * @param netmask Masque de sous-réseau IPv6 (ex: ffff:ffff:ffff:ffff:: -> 64)
 */
int ipv6_to_cidr(const char *netmask)
{
    struct sockaddr_in6 sa;
    if (inet_pton(AF_INET6, netmask, &sa.sin6_addr) == 1)
    {
        int cidr = 0;
        for (int i = 0; i < 16; i++)
        {
            unsigned char byte = sa.sin6_addr.s6_addr[i];
            while (byte)
            {
                if (byte & 0x80)
                {
                    cidr++;
                }
                byte <<= 1;
            }
        }
        return cidr;
    }
    return -1;
}

/**
 * @brief Récupère la liste des interfaces réseau de la machine.
 * @param interfaces Pointeur vers un tableau d'interfaces (à allouer par l'appelant)
 * @return Le nombre d'interfaces trouvées, ou -1 en cas d'erreur.
 */
int get_interfaces(struct Interface **interfaces)
{   

    struct ifaddrs *ifaddr, *ifa;
    int count = 0;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
            continue;

        int found = 0;
        for (int i = 0; i < count; i++)
        {
            if (strcmp((*interfaces)[i].name, ifa->ifa_name) == 0)
            {
                found = 1;
                if (ifa->ifa_addr->sa_family == AF_INET)
                {
                    add_ip_address(&(*interfaces)[i].ip4_addresses, &(*interfaces)[i].ip4_count, inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr));
                    strncpy((*interfaces)[i].netmask_v4, inet_ntoa(((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr), sizeof((*interfaces)[i].netmask_v4));
                }
                else if (ifa->ifa_addr->sa_family == AF_INET6)
                {
                    char ip6_address[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr, ip6_address, sizeof(ip6_address));
                    add_ip_address(&(*interfaces)[i].ip6_addresses, &(*interfaces)[i].ip6_count, ip6_address);
                    // Récupérer le masque pour IPv6 (si disponible)
                    if (ifa->ifa_netmask)
                    {
                        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)ifa->ifa_netmask)->sin6_addr, (*interfaces)[i].netmask_v6, sizeof((*interfaces)[i].netmask_v6));
                    }
                }
                break;
            }
        }
        if (!found)
        {
            *interfaces = realloc(*interfaces, (++count) * sizeof(struct Interface));
            struct Interface *interface = &(*interfaces)[count - 1];
            memset(interface, 0, sizeof(*interface));
            strncpy(interface->name, ifa->ifa_name, sizeof(interface->name));

            if (ifa->ifa_addr->sa_family == AF_INET)
            {
                add_ip_address(&interface->ip4_addresses, &interface->ip4_count, inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr));
                strncpy(interface->netmask_v4, inet_ntoa(((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr), sizeof(interface->netmask_v4));
            }
            else if (ifa->ifa_addr->sa_family == AF_INET6)
            {
                char ip6_address[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr, ip6_address, sizeof(ip6_address));
                add_ip_address(&interface->ip6_addresses, &interface->ip6_count, ip6_address);
                // Récupérer le masque pour IPv6 (si disponible)
                if (ifa->ifa_netmask)
                {
                    inet_ntop(AF_INET6, &((struct sockaddr_in6 *)ifa->ifa_netmask)->sin6_addr, interface->netmask_v6, sizeof(interface->netmask_v6));
                }
            }
        }
    }

    freeifaddrs(ifaddr);
    return count;
}

/**
 * @brief Libère la mémoire allouée pour les interfaces.
 */
void free_interfaces(struct Interface *interfaces, int count)
{
    for (int i = 0; i < count; i++)
    {
        for (int j = 0; j < interfaces[i].ip4_count; j++)
        {
            free(interfaces[i].ip4_addresses[j]);
        }
        free(interfaces[i].ip4_addresses);
        for (int j = 0; j < interfaces[i].ip6_count; j++)
        {
            free(interfaces[i].ip6_addresses[j]);
        }
        free(interfaces[i].ip6_addresses);
    }
    free(interfaces);
}

/**
 * @brief Génère une chaîne de caractères avec les informations des interfaces réseau.
 * @param interfaces Tableau d'interfaces
 * @param count Nombre d'interfaces
 * @param name Nom de l'interface à afficher (NULL pour toutes les interfaces)
 * @param sizeOut Pointeur vers la taille de la chaîne générée
 * @return Chaîne de caractères contenant les informations des interfaces (doit être libérée par l'appelant).
 */
char *interfaces_to_string(struct Interface *interfaces, int count, const char *name, size_t *sizeOut)
{
    // ========================
    // Interface: lo
    //   IPv4: 127.0.0.1/8
    //   IPv6: ::1/128
    // ========================
    // Interface: wlp0s20f3
    //   IPv4: 192.168.1.235/24
    //   IPv6: fdac:d2ed:6fc6:3c47:8c4a:137d:40d2:1429/64
    //   IPv6: fdac:d2ed:6fc6:3c47:b9:bac6:7b65:ad46/64
    //   IPv6: fe80::4ca8:80db:9f:c294/64
    // ========================
    // Calculer la taille de la chaîne à allouer
    size_t size = 0;
    size += strlen("========================\n");
    for (int i = 0; i < count; i++)
    {
        if (name == NULL || strcmp(interfaces[i].name, name) == 0)
        {
            size += strlen("Interface: ") + strlen(interfaces[i].name) + 1;
            for (int j = 0; j < interfaces[i].ip4_count; j++)
            {
                size += strlen("  IPv4: ") + strlen(interfaces[i].ip4_addresses[j]) + 4;
            }
            for (int j = 0; j < interfaces[i].ip6_count; j++)
            {
                size += strlen("  IPv6: ") + strlen(interfaces[i].ip6_addresses[j]) + 4;
            }
            size += strlen("========================\n");
        }
    }

    // Allouer la mémoire pour la chaîne
    char *info = malloc(size + 1);
    if (info == NULL)
    {
        return NULL;
    }
    char *ptr = info;

    // Générer la chaîne de caractères
    ptr += sprintf(ptr, "========================\n");

    for (int i = 0; i < count; i++)
    {
        if (name == NULL || strcmp(interfaces[i].name, name) == 0)
        {
            ptr += sprintf(ptr, "Interface: %s\n", interfaces[i].name);
            for (int j = 0; j < interfaces[i].ip4_count; j++)
            {
                ptr += sprintf(ptr, "  IPv4: %s/%d\n", interfaces[i].ip4_addresses[j], ipv4_to_cidr(interfaces[i].netmask_v4));
            }
            for (int j = 0; j < interfaces[i].ip6_count; j++)
            {
                ptr += sprintf(ptr, "  IPv6: %s/%d\n", interfaces[i].ip6_addresses[j], ipv6_to_cidr(interfaces[i].netmask_v6));
            }
            ptr += sprintf(ptr, "========================\n");
        }
    }
    if (sizeOut != NULL)
    {
        *sizeOut = size;
    }
    return info;
}

/**
 * @brief Génère une chaîne de caractères avec les informations d'une interface réseau.
 * @param interface Interface réseau
 * @param sizeOut Pointeur vers la taille de la chaîne générée
 * @return Chaîne de caractères contenant les informations de l'interface (doit être libérée par l'appelant).
 */

char *interface_to_string(struct Interface *interface, size_t *sizeOut)
{
    // Calculer la taille de la chaîne à allouer
    size_t size = 0;
    size += strlen("Interface: ") + strlen(interface->name) + 1;
    for (int j = 0; j < interface->ip4_count; j++)
    {
        size += strlen("  IPv4: ") + strlen(interface->ip4_addresses[j]) + 4;
    }
    for (int j = 0; j < interface->ip6_count; j++)
    {
        size += strlen("  IPv6: ") + strlen(interface->ip6_addresses[j]) + 4;
    }
    size += strlen("========================\n");

    // Allouer la mémoire pour la chaîne
    char *info = malloc(size + 1);
    if (info == NULL)
    {
        return NULL;
    }
    char *ptr = info;

    // Générer la chaîne de caractères
    ptr += sprintf(ptr, "Interface: %s\n", interface->name);
    for (int j = 0; j < interface->ip4_count; j++)
    {
        ptr += sprintf(ptr, "  IPv4: %s/%d\n", interface->ip4_addresses[j], ipv4_to_cidr(interface->netmask_v4));
    }
    for (int j = 0; j < interface->ip6_count; j++)
    {
        ptr += sprintf(ptr, "  IPv6: %s/%d\n", interface->ip6_addresses[j], ipv6_to_cidr(interface->netmask_v6));
    }
    ptr += sprintf(ptr, "========================\n");

    if (sizeOut != NULL)
    {
        *sizeOut = size;
    }
    return info;
}

/**
 * @brief Affiche les informations des interfaces réseau.
 * @param interfaces Tableau d'interfaces
 * @param count Nombre d'interfaces
 * @param name Nom de l'interface à afficher (NULL pour toutes les interfaces)
 */
void print_interfaces(struct Interface *interfaces, int count, const char *name)
{
    // Appeler la fonction qui génère la chaîne de caractères
    char *info = interfaces_to_string(interfaces, count, name, NULL);

    // Afficher le contenu généré
    printf("%s", info);

    free(info);
}

/**
 * @brief Récupère le nom d'hôte local.
 * @return Nom d'hôte local (doit être libéré par l'appelant).
 */
char *get_hostname()
{
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == -1)
    {
        perror("gethostname");
        return NULL;
    }
    return strdup(hostname);
}

/**
 * @brief Récupère l'adresse de broadcast d'une interface.
 * @param interface Interface réseau
 * @return Adresse de broadcast (doit être libérée par l'appelant).
 */
char* get_broadcast_address(const struct Interface interface)
{
    char broadcast_address[INET_ADDRSTRLEN];
    if (interface.ip4_count == 0)
    {
        return NULL;
    }
    struct in_addr ip_addr, netmask_addr;
    inet_pton(AF_INET, interface.ip4_addresses[0], &ip_addr);
    inet_pton(AF_INET, interface.netmask_v4, &netmask_addr);
    ip_addr.s_addr |= ~netmask_addr.s_addr;
    inet_ntop(AF_INET, &ip_addr, broadcast_address, sizeof(broadcast_address));
    return strdup(broadcast_address);
}


void send_broadcast_on_interfaces(const struct Interface *interfaces,const int PORT, int if_count, int hops, const struct Interface *interface_to_ignore, const char *message_ow) {
    int sockfd = initialize_socket(SOCK_DGRAM, 1); // Créer un socket pour le broadcast
    if (sockfd < 0) {
        return;
    }

    for (int i = 0; i < if_count; i++) {
        if (interfaces[i].ip4_count == 0 || strcmp(interfaces[i].name, "lo") == 0) {
            continue; // Ignorer les interfaces sans adresse IP ou l'interface locale
        }

        if (interface_to_ignore != NULL && strcmp(interfaces[i].name, interface_to_ignore->name) == 0) {
            continue; // Ignorer l'interface à ignorer
        }

        // Configurer l'adresse du serveur
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);  // Port arbitraire
        char *broadcast_address = get_broadcast_address(interfaces[i]);
        server_addr.sin_addr.s_addr = inet_addr(broadcast_address);

        // Envoyer un message de broadcast

        char message[64];
        if (message_ow == NULL) {
            sprintf(message, "%d@%s", hops, interfaces[i].ip4_addresses[0]);
        } else {
            sprintf(message, "%s", message_ow);
        }
        ssize_t sent_len = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sent_len < 0) {
            perror("Error sending broadcast");
        } else {
            printf("Broadcasting sur %s (%s)...%s\n", interfaces[i].name, broadcast_address, message);
        }
    }

    close(sockfd); // Fermer le socket après l'envoi des messages
}