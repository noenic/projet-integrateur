# 2 Alpine : Le terminal en mode graphique

## 2.1 Intaller Alpine 
- Dans un premier temps, On va prendre l'ISO d'Alpine en version [virtuelle](https://alpinelinux.org/downloads/)
- On va lancer la commande `setup-alpine` pour installer le système sur le disque dur
- On va choisir le clavier `fr` et le type de clavier `azerty`
- On change le hostname en `alpine`
- On initialise la carte réseau eth0 en DHCP
- Le mot de passe de root est `root`
- On va choisir le fuseau horaire `Europe/Paris`
- On va choisir de ne pas utiliser de proxy
- Pour le NTP, on va garder `Chrony`
- On va choisir le mirroir le plus rapide
- L'utilisateur est `user` avec le mot de passe `user`
- On va chosir le disque dur `sda` avec le partitionnement `sys`

Après l'installation : 63Mo

Instalation de l'interface graphique
- On commence avec `setup-xorg-base` 
- On install xfce4 `apk add xfce4 xfce4-terminal  lightdm-gtk-greeter`

- On ajoute les VMWare tools pour la gestion de la souris et du clavier 
```bash 
apk add open-vm-tools open-vm-tools-guestinfo open-vm-tools-deploypkg open-vm-tools-gtk xf86-input-vmmouse xf86-video-vmware
```

- On install les logicels requis (filezilla, tcpdump, wireshark,putty, nyxt) -> 1.2Go
```bash
apk add filezilla tcpdump wireshark putty nyxt
```

Install grub
```bash
apk add grub-bios
grub-install --target=i386-pc /dev/sda
```


`/etc/runlevels/nographics # cat /etc/grub.d/40_custom`
```bash
#!/bin/sh
exec tail -n +3 $0
# This file provides an easy way to add custom menu entries.  Simply type the
# menu entries you want to add after this comment.  Be careful not to change
# the 'exec tail' line above.


menuentry 'Alpine, console only' --class alpine --class gnu-linux --class gnu --class os {
        load_video
        insmod gzio
        insmod part_msdos
        insmod ext2
        set root='hd0,msdos1'
        search --no-floppy --fs-uuid --set=root 9f8649ba-9f0a-46c9-ad71-f22e3d0916eb
        echo    'Loading Linux virt with console only...'
        linux   /vmlinuz-virt root=UUID=c25e6521-08e4-4fe9-9585-45833f17b0f3 ro rootfstype=ext4 quiet text softlevel=nographics
        echo    'Loading initial ramdisk ...'
        initrd  /initramfs-virt
}

grub-mkconfig -o /boot/grub/grub.cfg

```

On ajoute un nouveau runlevel `nographics` pour démarrer sans interface graphique
```bash
mkdir -p /etc/runlevels/nographics
cp /etc/runlevels/default/* /etc/runlevels/nographics/
rc-update del lightdm  nographics
```
