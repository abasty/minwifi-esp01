# TODO

* [x] Passer tout en float
* [x] Coder `eval_factor`
* [x] Code `( expr )` dans factor ?
* [x] Gérer la virgule et l'exposant
* [x] Ajouter les fonctions numériques sur float SIN, COS, etc et PI, RND, CODE
* [x] Ajouter les variables
* [x] Ajouter RUN / CLEAR / NEW
* [x] Ajouter io (interface avec materiel)
* [x] printf: ansi / minitel
* [x] INPUT (envoie de caractères depuis le main vers le basic)
* [x] Variables strings
* [x] Expressions strings
* [x] SAVE / LOAD prog
* [x] SAVE / LOAD vars
* [ ] Transformation tree -> list (parcours GRD, etc) à mettre dans ds
* [ ] comparaison, condition
* [ ] IF, GOTO
* [ ] GOSUB, RETURN
* [ ] FOR, NEXT
* [ ] DOWNLOAD / UPLOAD
* [ ] Ajouter edit, integration "edit_min" ?
* [ ] vitesse serial ()
* [ ] Ajouter mode rouleau, mode 40/80, AT, INK, PAPER, co, coff, echo
* [x] print sur network
* [ ] TELNET / TELNET WS
* [ ] AT, INK, PAPER, CLS
* [ ] Tableaux (DIM)

# Liens

* <https://www.museeminitel.fr/>
* <https://www.minitel.org/>
* <http://pficheux.free.fr/xtel/>
* <https://www.tindie.com/products/iodeo/minitel-esp32-dev-board/>
* <https://forum.museeminitel.fr/t/minitel-esp32-carte-peri-informatique-wifi-ble/711/42>
* Basic ZX81 : <http://otremolet.free.fr/otnet/otzx/zx81/basic-progr/appxc.html>
* Sinclair Basic : <https://en.wikipedia.org/wiki/Sinclair_BASIC>

# Prise péri informatique

```
TX -     - RX
    / | \
 9v   0v  PT
```

Sur le fil DIN noir :

* TX : Bleu
* RX : Rouge
* Tresse : 0v

* 9v : Vert
* OT : Blanc

Sur la DIN, 3 fils souples papa :

* TX : Marron
* RX : Rouge
* 0v : Noir

# Target PlatformIO

* `[env:minwifi]` : À redéfinir, devra être le firmware_ota_only pour le Sonoff
  branché sur FTDI. À utiliser uniquement la 1ère fois pour reflasher le sonoff
  où quand OTA n'est plus accessible.

* `[env:minwifi_ota]` : Le firmware complet pour minitel uniquement. OTA ne
  permet pas de recharger ce même firmware. Transmis par OTA step 1.

* `[env:minwifi_ota_only]` : Le firmware avec juste OTA sans basic. À utiliser
  comme step 1 quand le firmware complet est trop gros. Transmis par OTA.

* `[env:esp01_1m]` : Un firmware complet pour un ESP01s branché directement un
  FTDI / programmateur. Il est transmis par USB Serial.


# Style C

```
$ astyle --style=1tbs -s4 src/*
```

# OTA

Procédure qui ne marche pas :

* Flash par USB serial => SW reboot de l'ESP
* Après le reboot => FOTA

Il faut **absolument** faire un HW reset de l'ESP :

* Flash par USB serial => SW reboot de l'ESP
* Débrancher / Rebrancher l'ESP => HW reboot
* FOTA fonctionne

# TCP Minitel

Lorsque une connexion WiFi existe avec un serveur, l'ESP passe en mode "Minitel" :
tout ce qui arrive sur `Serial` passe sur la _socket_ et inversement.

Pour tester la connexion, il suffit de lancer un `nc -l 2000` par exemple et
configurer l'ESP avec `configopt` puis `<IP du serveur>` et `<port du serveur>`.

Pour repasser en mode commande, il suffit de sortir de `nc`.

# WebSocket Minitel

* Web sockets et liens vers services sur IP :
<https://cq94.medium.com/retour-du-minitel-sur-le-web-8b8693ae8c6a>
* <http://3611.re/> : Dans `minitel-3611.js` on a l'URI de la WebSocket :
  `"ws://3611.re/ws"`

Test avec Python, on installe `sudo apt install python3-websockets`.

Service de test "echo" :

```sh
$ python3 -m websockets ws://echo.websocket.events
```

Sur un WebSocket Minitel on reçoit directement du videotex :

```
$ python3 -m websockets "ws://3611.re/ws"
```

Donc il suffit d'utiliser la lib WebSocket pour ESP8266 et le tour est joué.

# Database de service minitel

* À voir avec minitel.org ou cq94.

# Pour l'article

* pio dans vscode (menus)
* sinon il faut avoir pio en ligne de commande : `source ~/.platformio/penv/bin/activate`

```
$ pio run --list-targets
$ pio device monitor
$ pio run -e minwifi
$ pio run -e minwifi -t clean
```

# Basic ZX81 like

## Tokens

## Valeurs

* réels sur 4 octets : Utiliser IEEE_754 : <https://fr.wikipedia.org/wiki/IEEE_754>
* string

## Expressions

## Variables

### Déclaration et réservation mémoire

* Symbole

Un `$` est accepté à la fin pour signifier "string". Les symboles sont stockés
dans un arbre binaire, indexé par la valeur du symbole.

* Valeur

Pour les chaines de caractères, la valeur associée au symbole est un pointeur.
Pour les valeurs nombre (float), c'est la valeur en forme token (le token + la
valeur tokenisée).

* Tableaux

Stocker les dimensions et le pointeur, vers la zone mémoire allouée avec calloc.
4  dimensions max ?.

## Ligne de prog

Les lignes sont stockées dans un arbre binaire indexé par le n0 de ligne (uint16_t).

## Sauvegarde

Les lignes de prog et les variables sont sauvegardées. Un parcours GRD suffit
pour sérialiser.

Pour le prog, chaque ligne est sauvegardée selon le schéma suivant :

* n° de ligne (uint16_t)
* longueur de la ligne (uint16_t)
* ligne tokenizée

Visualiser un fichier sauvegardé :

```
$ hexdump -C toto
```

## Mémoire
