
# 3 MicroCore : Le terminal en mode console.

## 3.1 Installer MicroCore

On télécharge l'ISO de MicroCore sur le site officiel de TinyCore: [http://tinycorelinux.net/](http://tinycorelinux.net/)
On lance `tce-load -wi install.tcz` pour l'installer

## 3.2 Intéger automatiquement le clavier azerty
On ajoute cette ligne dans `/opt/bootsync.sh`
```bash
loadkmap < /usr/share/kmap/azerty/fr-latin9.kmap
```

## 3.3 Configurer les dossiers utilisateurs (/home) et logiciels (/opt) afin qu'ils soient persistants
On ajoute ces options de boot dans le fichier de configuration ``/mnt/sda1/tce/boot/extlinux/extlinux.conf``
```bash
APPEND ... opt=sda1 home=sda1 ... 
```

## 3.4 Integrer à minima IPv6, les commandes Linux `ip` et `tcpdump`

On va utiliser le gestionnaire de paquets TinyCore: `tce-load` pour installer les paquets `iproute2`, `tcpdump` et `ipv6-netfilter-6.6.8-tinycore.tcz` (pour activer IPv6)
```bash	
tce-load -wi iproute2 tcpdump ipv6-netfilter-6.6.8-tinycore.tcz
```

Pour activer IPv6, on ajoute ces lignes dans `/opt/bootsync.sh`
```bash
modprobe ipv6
```