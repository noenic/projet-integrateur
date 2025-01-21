// ifshow/main.c

#include <stdio.h>
#include "../iflib/lib_interfaces.h"
#include <string.h>

int main(int argc, char *argv[]) {
    struct Interface *interfaces = NULL;
    int count = get_interfaces(&interfaces);

    if (count < 0) {
        return 1; // Erreur lors de la récupération des interfaces
    }

    if (argc == 2 && strcmp(argv[1], "-a") == 0) {
        print_interfaces(interfaces, count, NULL);

    } else if (argc == 3 && strcmp(argv[1], "-i") == 0) {
        print_interfaces(interfaces, count, argv[2]);
    } else {
        printf("Usage: %s [-a] [-i <interface>]\n", argv[0]);
    }

    // Libérer la mémoire allouée
    free_interfaces(interfaces, count);
    return 0;
}
