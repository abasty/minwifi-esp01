# Test

Lorsque une connexion WiFi existe avec un serveur, l'ESP passe en mode "Minitel"
: tout ce qui arrive sur `Serial` passe sur la _socket_ et inversement.

Pour tester la connexion, il suffit de lancer un `nc -l 2000` par exemple et
configurer l'ESP avec `configopt` puis `<IP du serveur>` et `<port du serveur>`.

Pour repasser en mode commande, il suffit de sortir de `nc`.
