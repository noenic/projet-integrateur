#ifndef LIB_INTERFACES_H
#define LIB_INTERFACES_H

#include <netinet/in.h>

// Définition de la structure `Interface`
struct Interface {
    char name[32];         // Nom de l'interface (ex: eth0, lo)
    char **ip4_addresses;  // Liste des adresses IP de l'interface (IPv4)
    int ip4_count;         // Nombre d'adresses IPv4
    char **ip6_addresses;  // Liste des adresses IP de l'interface (IPv6)
    int ip6_count;         // Nombre d'adresses IPv6
    char netmask_v4[64];   // Masque de sous-réseau IPv4 (en format classique)
    char netmask_v6[64];   // Masque de sous-réseau IPv6 (en format classique)
};



// Déclarations des fonctions
void add_ip_address(char ***addresses, int *count, const char *address);
int initialize_socket(int type, int broadcast);
int ipv4_to_cidr(const char *netmask);
int ipv6_to_cidr(const char *netmask);
int get_interfaces(struct Interface **interfaces);
void free_interfaces(struct Interface *interfaces, int count);

// Fonction qui génère une chaîne de caractères contenant les informations des interfaces
char* interfaces_to_string(struct Interface *interfaces, int count, const char *name, size_t *sizeOut);
char* interface_to_string(struct Interface *interface, size_t *sizeOut);

// Fonction d'affichage des interfaces réseau
void print_interfaces(struct Interface *interfaces, int count, const char *name);

// Fonction pour obtenir le nom d'hôte local
char* get_hostname();     


// Fonction pour obtenir l'adresse de broadcast d'une interface 
char* get_broadcast_address(const struct Interface interface);



// Fonctions spécifiques à neighborshow
void send_broadcast_on_interfaces(const struct Interface *interfaces,const int PORT, int if_count, int hops, const struct Interface *interface_to_ignore, const char *message_ow);



#endif // LIB_INTERFACES_H
