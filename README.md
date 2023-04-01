# Liens

* <https://www.museeminitel.fr/>
* <https://www.minitel.org/>
* <http://pficheux.free.fr/xtel/>

# Prise péri informatique

## Vue extérieur de derrière


TX -     - RX
    / | \
 9v   0v  PT


Sur le fil DIN noir :

TX : Bleu
RX : Rouge
Tresse : 0v

9v : Vert
OT : Blanc

Sur la DIN, 3 fils souples papa :

TX : Marron
RX : Rouge
0v : Noir

# Style C

```
$ astyle --style=1tbs -s2 src/*
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
$ pio --list-targets
$ pio device monitor
$ pio run -e minwifi
$ pio run -e minwifi -t clean
```
