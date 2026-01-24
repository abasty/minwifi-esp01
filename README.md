# 1. Sortir du mode connecté ou d'une exécution Basic

Appuyer 2 fois sur la touche "ESC", si ça ne marche pas, appuyer sur le bouton
RESET. Si on ne veut pas exécuter le programme `autoexec.bas` au démarrage, un
appui long sur le bouton RESET permet de l'éviter.

# 2. Se connecter à un réseau Wi-Fi

Pour voir les réseaux connus :

```
WIFI LIST
```

Si le réseau Wi-Fi est connu, on se connecte avec `WIFI "<nom du réseau>"`.
Sinon :

La première fois :

```
WIFI SCAN
WIFI <n° du réseau>
<Entrer le mot de passe en ligne 0 comme demandé>
OK
```

Voir _Troubleshooting_ si les caractères ne sont pas accessibles depuis le
clavier.

Les fois suivantes :

```
WIFI "<nom du réseau>"
OK
```

# 3. Se connecter au FTP BASTOS

Il faut être connecté au Wi-Fi.

Pour voir les FTP déjà configurés :

```
FTP LIST
```

Si le site "bastos" n'est pas configuré :

La première fois :

```
FTP "bastos", "ftp:abasty-retro.fr:2121:bastos"
OK
```

Les fois suivantes :

```
FTP "bastos"
OK
```

# 4. Télécharger et exécuter "snake.bas"

Il faut être connecté au FTP BASTOS.

Pour voir le disque distant FTP : `FTP CAT`. Pour télécharger "snake.bas" :

```
FTP GET "snake.bas"
OK
```

Le fichier `snake.bas` est téléchargé depuis le FTP BASTOS et sauvegardé sur le
disque local.

Il n'est pas nécessaire d'être connecté pour exécuter des programmes Basic sur
le disque local. Le cas échéant on peut vouloir se déconnecter du FTP et/ou du
Wi-Fi :

```
FTP STOP
WIFI STOP
```

Pour voir le disque local : `CAT`.


Pour charger et exécuter "snake.bas" depuis le disque local :

```
RUN "snake"
```

# 5. Se connecter à des services Minitel

Il faut être connecté au Wi-Fi.

La connexion aux services Minitel présents sur Internet se fait par TCP,
éventuellement avec une sur-couche WebSocket. Certains services purement RTC,
aujourd'hui sur VoIP, sont néanmoins accessibles à BASTOS par le service
Internet "Minipavi" de Jean-Arthur qui agit en temps que passerelle.

Merci Jean-Arthur !

Pour voir les services Minitel déjà configurés :

```
MINITEL LIST
```

Pour se connecter à un service non configuré :

```
MINITEL "<nom du service>","<URN>"
```

Exemples :

* `MINITEL "minipavi", "tcp:go.minipavi.fr:516"`
* `MINITEL "3611", "ws:3611.re:80:/ws"`
* `MINITEL "3615", "ws:3615co.de:80:/ws"`
* `MINITEL "hacker", "ws:mntl.joher.com:2018:/?echo"`
* `M̀INITEL "rcampus", "tcp:bbs.retrocampus.com:1651"`

Pour se connecter à un service déjà configuré :

```
MINITEL "<nom du service>"
```

# Troubleshooting

## Caractères spéciaux dans références Wi-Fi

Si le SSID et/ou le mot de passe d'un réseau Wi-Fi contiennent des caractères
non accessibles depuis le clavier, il est possible d'entrer ses données en
utilisant des chaines de caractères contenant des codes hexadécimaux directement
dans la table des réseaux Wi-Fi connus.

Les codes hexadéciamux dans les chaines de caractères ont la forme `\xXY`.

La table des réseaux Wi-Fi connus porte le numéro 255 dans la base de données
auto-sauvegardée de BASTOS.

Ainsi la commande `PUT 255,"\x41\x62\x61", "to\x75\x6f"` définira le réseau de
SSID "Aba" et de mot de passe "toto".

`WIFI LIST` ou `DB LIST 255` affichent la table 255.

`WIFI "\x41\x62\x61"`, ou `WIFI "Aba"` dans cet exemple permettront de se
connecter au réseau.

## Tout est planté

* Pour sortir d'un programme en exécution ou du mode connecté, appuyer 2 fois
  consécutivement sur ESC
* En connexion sur un service Minitel la touche "Connexion/Fin" peut aider.
* La commande `BASTOS` permet de redéfinir les attributs par défaut pour BASTOS
  et afficher l'invite de commande initiale
* La commande `RESET` permet de réinitialiser BASTOS et de revenir en mode
  `SLOW` (prise à 1200 bps)
* Un appui court sur le bouton RESET est équivalent à la commande `RESET`
* Un appui long sur le bouton RESET fait `RESET` **et** n'exécute pas
  `autoexec.bas` au re-démarrage
* La suppression du fichier `autoexec.bas` peut aussi aider à sortir d'une
  boucle infernale (de la même façon que l'appui long sur RESET)
* Finalement en ultime : Débrancher le module BASTOS et le rebrancher de/sur la
  DIN du Minitel
