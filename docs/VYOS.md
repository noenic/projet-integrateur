# 1 Vyos : Le routeur Virtuel
## 1.2 Integrer automatiquement le clavier azerty
Il suffit d'ajouter cette ligne dans `/config/scripts/vyos-preconfig-bootup.script`
```bash
loadkeys fr
```

## 1.3 Homohénéiser les interfaces réseaux
Pour cela ils nous suffit de créer la VM sans carte réseau, par la suite on pourra la dupliquer et ajouter les cartes réseaux nécessaires sur les VMs dupliquées.
C'est possible car nous n'avons pas besoin de connexions réseau pour la configuration de Vyos avec les consignes données.


## 1.4 Configuration des interfaces réseaux

# R0
R0 a 3 interfaces: 
- eth0 (10.17.1.1/24, 2001:0:17:1::1/64) dans le réseau N1
- eth1 (10.17.2.1/24, 2001:0:17:2::1/64) dans le réseau N2
- eth2 (10.17.5.2/24, 2001:0:17:5::2/64) dans le réseau N5

```bash
configure
set system host-name R0
set interfaces ethernet eth0 address 10.17.1.1/24
set interfaces ethernet eth0 address 2001:0:17:1::1/64

set interfaces ethernet eth1 address 10.17.2.1/24 
set interfaces ethernet eth1 address 2001:0:17:2::1/64

set interfaces ethernet eth2 address 10.17.5.2/24
set interfaces ethernet eth2 address 2001:0:17:5::2/64

# OSPF
set protocols ospf parameters router-id 0.0.0.0
set protocols ospf interface eth0 area 0
set protocols ospf interface eth1 area 0
set protocols ospf interface eth2 area 0
set protocols ospf interface eth0 passive
set protocols ospf interface eth1 passive

# OSPFv3
set protocols ospfv3 parameters router-id 0.0.0.0
set protocols ospfv3 interface eth0 area 0
set protocols ospfv3 interface eth1 area 0
set protocols ospfv3 interface eth2 area 0
set protocols ospfv3 interface eth0 passive
set protocols ospfv3 interface eth1 passive


# On active le router advertisement sur les interfaces eth0 et eth1
set service router-advert interface eth0
set service router-advert interface eth1
set service router-advert interface eth0 prefix 2001:0:17:1::/64
set service router-advert interface eth1 prefix 2001:0:17:2::/64



```



# R1
R1 a 3 interfaces: 
- eth0 (172.16.0.17/24 , 2002:16:0:0::17/64) dans le réseau N0
- eth1 (192.168.17.5/30, 3FFE:0:17:4::1/64) dans le réseau N4
- eth2 (10.17.5.1/24, 2001:0:17:5::1/64) dans le réseau N5

```bash
configure
set system host-name R1
set interfaces ethernet eth0 address 172.16.0.17/24
set interfaces ethernet eth0 address 2002:16:0:0::17/64

set interfaces ethernet eth1 address 192.168.17.5/30 
set interfaces ethernet eth1 address 3FFE:0:17:4::1/64

set interfaces ethernet eth2 address 10.17.5.1/24
set interfaces ethernet eth2 address 2001:0:17:5::1/64

# OSPF
set protocols ospf parameters router-id 1.1.1.1
set protocols ospf interface eth0 area 0
set protocols ospf interface eth1 area 0
set protocols ospf interface eth2 area 0

# OSPFv3
set protocols ospfv3 parameters router-id 1.1.1.1
set protocols ospfv3 interface eth0 area 0
set protocols ospfv3 interface eth1 area 0
set protocols ospfv3 interface eth2 area 0


```


# R2
R2 a 2 interfaces: 
- eth0 (10.17.3.1/24 , 2001:0:17:3::1/64) dans le réseau N3
- eth1 (192.168.17.6/30, 3FFE:0:17:4::2/64) dans le réseau N4

```bash
configure
set system host-name R2
set interfaces ethernet eth0 address 10.17.3.1/24
set interfaces ethernet eth0 address 2001:0:17:3::1/64

set interfaces ethernet eth1 address 192.168.17.6/30
set interfaces ethernet eth1 address 3FFE:0:17:4::2/64

# OSPF
set protocols ospf parameters router-id 2.2.2.2
set protocols ospf interface eth0 area 0
set protocols ospf interface eth1 area 0
set protocols ospf interface eth0 passive

# OSPFv3
set protocols ospfv3 parameters router-id 2.2.2.2
set protocols ospfv3 interface eth0 area 0
set protocols ospfv3 interface eth1 area 0
set protocols ospfv3 interface eth0 passive

set service router-advert interface eth0
set service router-advert interface eth0 prefix 2001:0:17:3::/64

```





# CLOUD
C'est un routeur temporaire pour avoir internet
- eth0 (dhcp) dans le réseau N0
- eth1 (172.168.0.1/24) dans le réseau N0

```bash
configure
set system host-name CLOUD
set interfaces ethernet eth0 address dhcp
set interfaces ethernet eth1 address 172.16.0.1/24

# NAT
set nat source rule 10 source address '10.17.0.0/16'
set nat source rule 10 translation address masquerade


# OSPF
set protocols ospf parameters router-id 9.9.9.9
set protocols ospf interface eth0 area 0
set protocols ospf interface eth1 area 0
# On met la route par défaut dans OSPF
set protocols ospf  default-information originate always
```
