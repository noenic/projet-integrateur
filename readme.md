# 5.3-Projet-Integrateurs

Dépot pour le projet intégrateur du semestre 5

## Documentation 
Les différentes documentations se trouvent dans le dossier `docs` à la racine du projet.
- [VYOS](docs/VYOS.md)
- [Microcore](docs/Microcore.md)
- [Alpine](docs/Alpine.md)

## Les programmes
Les différents programmes ont été installés sur les machines virtuelles.
Les scripts utilisent une librairie partagée `iflib` qui contient des fonctions redondantes.

### Ifshow
Le programme ``ifshow`` sert à afficher la liste des interfaces réseau d'une machine, en fournissant leurs noms, adresses et suffixes.

```bash
Usage: ./bin/ifshow [-a] [-i <interface>]

 -i <interface> : affiche les informations de l'interface spécifiée (ex: eth0)
 -a : affiche toutes les interfaces
```


### Ifnetshow 
Le programme ``ifnetshow`` sert à afficher les informations d'une interface réseau d'une machine distante.

Il dispose de deux exécutables:
- `ifnetshow-client` : se connecte à un serveur distant et affiche les informations de l'interface spécifiée
- `ifnetshow-server` : attend une connexion d'un client et affiche les informations de l'interface spécifiée

En soit le serveur est pratiquement identique a `ifshow` sauf qu'au lieu d'afficher les informations dans la console, il les envoie au client par un socket.
arguments:

```bash
Usage: ./bin/ifnetshow-client -n <server_ip_or_hostname> (-i <interface_name> | -a for all)

 -n <server_ip_or_hostname> : adresse ip ou nom de l'hôte du serveur
 -i <interface> : affiche les informations de l'interface spécifiée (ex: eth0)
 -a : affiche toutes les interfaces
```



### Neighborshow
Le programme ``neighborshow`` sert à afficher le nom des machines voisines.

Il dispose de deux exécutables:
- `nbsc` : Envoie un broadcast sur le réseau et affiche les machines qui ont répondu
- `nbsd` : Attend un broadcast et répond avec son nom (relais le broadcast si c'est un routeur)

```bash
./nbsc
 -hops <n> : nombre de sauts maximum pour le broadcast (default: 1)
```

```bash
./nbsd
 -r : relais le broadcast sur les autres interfaces (default: false)
```

### Compilation

Pour pouvoir etre executables sur les 3 systèmes, le compilateur utilisé est `musl-gcc` qui est une version de gcc qui utilise la librairie musl au lieu de glibc.

Il faut aussi ajouter le flag `-m32` pour compiler en 32 bits (Microcore)

Normalement, il suffit de faire un `make` pour compiler les programmes, assurez vous d'avoir `make` et `musl-gcc` d'installés.
```bash
make -C src
```

Les binaires seront dans le dossier `bin`


