---
title: Guide de développement
---

# Guide de développement BASTOS

Ce guide explique comment écrire et tester des programmes BASTOS sur un PC
(Windows ou Linux), sans matériel Minitel ni module BASTOS-S.

Le principe : un simulateur `bastos` tourne sur la machine et expose une
WebSocket locale. Minterm s'y connecte et affiche l'invite BASTOS envoyée par le
simulateur. Il est alors possible d'utiliser BASTOS depuis l'émulateur comme on
le ferait avec un module BASTOS-S depuis un Minitel.

Pour la syntaxe du langage (instructions, fonctions, commandes disque et
réseau...), voir le [Manuel du langage BASTOS](BASTOS-MANUAL-fr).

## 1. Télécharger et lancer BASTOS

### Téléchargement

Les archives sont publiées sur le serveur FTP du projet, dossier `firmware` :

- **Linux** : `ftp://anonymous:@abasty-retro.fr:2121/firmware/bastos-linux-amd64.tar.gz`
- **Windows** : `ftp://anonymous:@abasty-retro.fr:2121/firmware/bastos-windows-amd64.zip`

Sous Windows, il est possible de coller directement cette adresse dans la barre
d'adresse de l'Explorateur de fichiers pour parcourir le dossier et
télécharger le `.zip` en cliquant dessus. Sous Linux comme sous Windows, en
ligne de commande :

```sh
curl -o bastos-linux-amd64.tar.gz "ftp://anonymous:@abasty-retro.fr:2121/firmware/bastos-linux-amd64.tar.gz"
```

```bat
curl -o bastos-windows-amd64.zip "ftp://anonymous:@abasty-retro.fr:2121/firmware/bastos-windows-amd64.zip"
```

Chaque archive contient :

- le binaire `bastos` (l'interpréteur BASTOS pour PC),
- un script de lancement (`bastos-back-linux.sh` / `bastos-back-windows.bat`),
- un dossier `disk` avec quelques programmes d'exemple.

### Lancer le serveur

Extraire l'archive, puis lancer le script correspondant à la plateforme :

**Linux**

```sh
tar xzf bastos-linux-amd64.tar.gz
cd bastos-linux-amd64   # ou le nom du dossier extrait
./bastos-back-linux.sh
```

**Windows**

Double-clic sur bastos-back-windows.bat, ou en ligne de commande :

```bat
bastos-back-windows.bat
```

Le script s'appuie sur [websocat](https://github.com/vi/websocat) pour
exposer BASTOS sur une WebSocket. S'il n'est pas déjà installé sur la
machine, le script propose de le télécharger automatiquement (répondre `o`) —
il est alors placé à côté du script, aucune installation système n'est
nécessaire.

Une fois lancé, le script affiche :

```
BASTOS lancé sur ws://127.0.0.1:1967
Lien Minterm : https://abasty.github.io/minterm/?ws=ws%3A%2F%2F127.0.0.1%3A1967
```

### Se connecter avec minterm

Le lien affiché ouvre directement minterm connecté au serveur BASTOS
local (le paramètre `?ws=...` pré-remplit l'adresse de connexion).

- **Sous Linux**, un `Ctrl+clic` sur le lien dans la plupart des terminaux
  (GNOME Terminal, Konsole...) l'ouvre directement dans le navigateur.
- **Sous Windows**, le comportement dépend du terminal utilisé (à vérifier
  selon les versions de Windows Terminal / invite de commandes classique) —
  si le clic ne fonctionne pas, il suffit de sélectionner puis copier-coller
  l'URL dans le navigateur.

Laisser le script tourner dans son terminal : c'est lui qui fait le pont
entre minterm et l'interpréteur BASTOS. Fermer le terminal coupe la
connexion.

### Explorer les exemples avec CAT

Une fois connecté, la commande `CAT` liste les fichiers du dossier `disk` :

```
CAT

Drive: A

autoload        .db   394
bastos          .bas  224
bastos          .txt  2039
connect         .bas  2924
craps           .bas  581
fx              .bas  272
meteor          .bas  7792
pgcd            .bas  147
sin             .bas  271
snake           .bas  3266

468K free
```

Quelques exemples à essayer avec `RUN "nom.bas"` :

- `craps.bas` : jeu de dés
- `snake.bas` : le jeu du serpent
- `sin.bas` : tracé d'une sinusoïde en mode graphique
- `meteor.bas` : jeu d'arcade
- `pgcd.bas` : calcul de PGCD/PPCM (démonstration `INPUT`)
- `bastos.bas` : Lancement de BASTOS

## 2. Éditer et créer ses propres programmes

Le dossier `disk` à côté du script (créé automatiquement au premier
lancement s'il n'existe pas) est le disque virtuel de BASTOS : tout fichier
qui s'y trouve est visible par `CAT`, chargeable par `LOAD`/`RUN`, et tout
programme sauvegardé depuis BASTOS (`SAVE "nom.bas"`) y est écrit.

Cycle de développement :

1. Éditer un fichier `.bas` du dossier `disk` avec un éditeur de texte (ou
   en créer un nouveau),
2. Sauvegarder,
3. Dans minterm, charger et lancer le programme :

   ```
   RUN "nom.bas"
   ```

4. Répéter — pas besoin de relancer le serveur `bastos` ni minterm entre
   deux modifications, seul le fichier sur disque doit être à jour avant le
   `RUN`.

## 3. Éditer confortablement avec VSCode

Le dépôt fournit une extension VSCode pour BASTOS : coloration syntaxique,
snippets, renumérotation des lignes, mise en forme automatique et
renommage de variable.

**[Documentation et téléchargement de l'extension](BASTOS-VSCODE-EXTENSION-fr)**

Usage typique :

1. Ouvrir le dossier `disk` de l'archive téléchargée dans VSCode (`Fichier
   > Ouvrir un dossier...`),
2. Créer ou modifier un fichier `.bas` — la coloration syntaxique s'active
   automatiquement,
3. Utiliser les snippets pour aller plus vite (taper `for` puis `Tab` pour
   une boucle `FOR...NEXT`, `if`/`ifelse` pour un test, `gosub`/`label`
   pour une sous-routine...),
4. Sauvegarder, puis dans minterm : `RUN "nom.bas"`.

`Maj+Alt+F` met en forme le fichier (mots-clés en majuscules, variables en
minuscules), et `F2` sur une variable ou une étiquette la renomme partout
dans le fichier.

## 4. Transférer un programme vers un module BASTOS-S (FTP)

Une fois un programme au point sur PC, on peut le récupérer directement sur un
vrai module BASTOS-S (connecté en WiFi) avec `FTP GET` : le module se connecte
en FTP au PC et télécharge le fichier depuis le dossier `disk`.

### Servir le dossier `disk` en FTP

Cette partie suppose un PC Linux comme serveur FTP (c'est ce qu'utilise le
projet lui-même). Sous Windows, il faudra un autre serveur FTP — non
couvert ici.

[vsftpd](https://security.appspot.com/vsftpd.html) est recommandé — il est
déjà utilisé pour le serveur FTP officiel du projet, et son comportement est
testé et compatible avec le client FTP de BASTOS. (Certains serveurs FTP
stricts, comme `pyftpdlib`, refusent l'ordre de commandes utilisé par
BASTOS — `SIZE` avant `TYPE I` — avec une erreur `550 SIZE not allowed in
ASCII mode` ; vsftpd l'accepte sans problème.)

```sh
sudo apt install vsftpd
```

Créer un fichier de configuration `vsftpd-dev.conf`, en pointant `anon_root`
vers le dossier `disk` du projet (chemin absolu) :

```
listen=YES
listen_port=2121
anonymous_enable=YES
anon_root=/chemin/absolu/vers/disk
write_enable=NO
pasv_enable=YES
pasv_min_port=10090
pasv_max_port=10100
```

Lancer le serveur (nécessite les droits root pour écouter sur le réseau) :

```sh
sudo vsftpd vsftpd-dev.conf
```

Le dossier `disk` est alors accessible en lecture seule par n'importe quel
appareil du même réseau local — pratique pour un poste de dev, à éviter sur
un réseau partagé ou non maîtrisé.

### Récupérer un programme depuis le module

Le module doit être sur le même réseau WiFi que le PC. Trouver l'adresse IP
locale du PC (`hostname -I` sous Linux, `ipconfig` sous Windows), par
exemple `192.168.1.42`, puis depuis BASTOS (sur le module, ou dans
l'émulateur pour tester) :

```basic
FTP "pc", "ftp:192.168.1.42:2121"
FTP GET "nom.bas"
FTP STOP
```

`FTP GET` récupère le fichier sous le même nom dans le dossier `disk` local
du module (ou de l'émulateur). Il ne reste plus qu'à faire `RUN "nom.bas"`.

Voir la section [FTP du manuel](https://abasty.github.io/minwifi-esp01/BASTOS-MANUAL-fr#ftp)
pour les autres commandes (`FTP CAT`, `FTP PUT`, connexions sauvegardées...).
